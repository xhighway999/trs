
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "args.hxx"
#include "miniz.h"
#include "nlohmann/json.hpp"
namespace fs = std::filesystem;

struct TrcFile {
  TrcFile() = default;
  TrcFile(const fs::path& p, std::string_view buildType) {
    parseFile(p, buildType);
  }
  void parseFile(const fs::path& p, std::string_view buildType) {
    std::ifstream t(p);
    std::stringstream buffer;
    buffer << t.rdbuf();
    nlohmann::json j = nlohmann::json::parse(buffer);
    projectName = j["project"];
    for (const auto& config : j["configs"]) {
      if (config["buildType"].get<std::string>() == "All" || config["buildType"].get<std::string>() == buildType) {
        fs::path resourcepath = p.parent_path() / config["folder"].get<std::string>();
        if (!fs::exists(resourcepath)) {
          throw std::runtime_error("Resourcepath '" + resourcepath.string() +
                                   "' does not exist");
        }
        resourcePaths.push_back(resourcepath);
      }
    }
  }
  void copy(const fs::path& target) {
    for (const auto& resourcePath : resourcePaths) {
      for (auto& p : fs::recursive_directory_iterator(resourcePath)) {
        if (p.is_regular_file()) {
          auto relPath =
              fs::path(projectName) / fs::relative(p, resourcePath).c_str();
          auto finPath = target / relPath;
          fs::create_directories(finPath.parent_path());
          fs::copy(p, finPath, fs::copy_options::update_existing);
        }
      }
    }
  }

 private:
  std::string projectName;
  std::vector<fs::path> resourcePaths;
};
static std::vector<TrcFile> trcFiles;

void createDirStructure(const fs::path& start,
                        const fs::path& end,
                        std::string buildType) {
  for (auto& p : fs::recursive_directory_iterator(start))
    if (p.path().extension() == ".trc") {
      try {
        trcFiles.emplace_back(p.path(), buildType);
      } catch (std::exception& e) {
        std::cerr << "Error in File " << p.path() << ": " << std::endl;
        std::cerr << e.what();
        throw;
      }
    }
  for (auto& trcf : trcFiles) {
    trcf.copy(end);
  }
}

void createZipFromPath(const fs::path& start, const fs::path& end) {
  mz_zip_archive archive{};
  mz_zip_writer_init_file(&archive, end.string().c_str(), 0);

  for (auto& p : fs::recursive_directory_iterator(start)) {
    if (p.is_regular_file()) {
      auto relative = fs::relative(p.path(), start);
      mz_zip_writer_add_file(&archive, relative.string().c_str(), p.path().string().c_str(),
                             nullptr, 0, MZ_BEST_COMPRESSION);
    }
  }

  mz_zip_writer_finalize_archive(&archive);
  mz_zip_writer_end(&archive);
}

void fileToC(const fs::path& start,
             const fs::path& end,
             std::string_view name,
             size_t width) {
  std::ifstream t(start,std::ifstream::binary);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string out;

  out += "unsigned char ";
  out += name;
  out += "[] = {\n";

  std::string inStr = buffer.str();
  size_t lineBreak = 0;
  for (size_t i = 0; i < inStr.size() - 1; ++i) {
    char c = inStr[i];
    auto valString = std::to_string(static_cast<unsigned char>(c));
    valString += ",";
    valString.resize(4, ' ');
    out.append(valString);
    lineBreak++;
    if (width == lineBreak) {
      out += '\n';
      lineBreak = 0;
    }
  }
  out += std::to_string(static_cast<unsigned char>((inStr.back())));
  out += "};\n";
  out += "unsigned long long ";
  out += name;
  out += "_len = " + std::to_string(inStr.size()) + ";\n";

  std::ofstream offile(end, std::ofstream::binary | std::ofstream::trunc);
  offile.write(out.data(), static_cast<long int>(out.size()));
}

enum ResourceType { Folder, Archive, C };

int main(int argc, char** argv) {
  args::ArgumentParser parser("Tool for generating resources");
  args::Group group(
      parser, "This group requires in and out:", args::Group::Validators::All);

  std::unordered_map<std::string, ResourceType> resourceMap{
      {"folder", ResourceType::Folder},
      {"archive", ResourceType::Archive},
      {"c", ResourceType::C}};

  args::ValueFlag<std::string> inDir(group, "path", "the root project path",
                                     {"path", 'i'});
  args::ValueFlag<std::string> outDir(group, "out", "the output directory",
                                      {"out", 'o'});
  args::ValueFlag<std::string> name(
      parser, "name", "name for the generated c code", {"name", 'n'}, "data");
  args::ValueFlag<size_t> width(
      parser, "width",
      "number of characters per line in the generated c source", {"width", 'w'},
      64);
  args::ValueFlag<std::string> buildType(parser, "buildType",
                                         "specifies the build type",
                                         {"buildtype", 'b'}, "Debug");
  args::MapFlag<std::string, ResourceType> type(
      parser, "type",
      "specifies the output format; Options: folder (default), archive, c",
      {"type", 't'}, resourceMap, ResourceType::Folder);
  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help&) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }
  auto in = fs::absolute(inDir.Get());
  auto out = fs::absolute(outDir.Get());
  switch (type.Get()) {
    case ResourceType::Folder:
      createDirStructure(in, out, buildType.Get());
      break;
    case ResourceType::Archive:
      createDirStructure(in, fs::absolute("./tmp/"), buildType.Get());
      fs::remove(fs::absolute(out));
      createZipFromPath(fs::absolute("./tmp/"), out);
      fs::remove_all("./tmp/");
      break;
    case ResourceType::C:
      createDirStructure(in, fs::absolute("./tmp/"), buildType.Get());
      createZipFromPath(fs::absolute("./tmp/"), fs::absolute("./tmpfile.zip"));
      fileToC(fs::absolute("./tmpfile.zip"), out, name.Get(), width.Get());
      fs::remove(fs::absolute("./tmpfile.zip"));
      fs::remove_all("./tmp/");
      break;
  }
  return 0;
}
