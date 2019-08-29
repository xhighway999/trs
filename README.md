# Tiny Resource System

Trs is a small command line utility for finding resources and copying them to a path/archive/c source file. Contrary to other resource systems (Qt, Microsoft Resource Files) it is not bound to any particular framework.

## Getting Started

Be aware that this tool is still in development and the api / format can change at any point.



### Usage
Trs works by recursively searching for .trc files. A trc file is json code that contains the project name and the path to the resources.

A minimal trc file example:

```
{
  "project": "SOMENAMEHERE",
  "configs": [
    {
      "buildType": "All",
      "folder": "resources"
    }
  ]
}
```
If you want to specify additional resources for a debug build:
```
{
  "project": "SOMENAMEHERE",
  "configs": [
    {
      "buildType": "All",
      "folder": "resources"
    },
    {
      "buildType": "Debug",
      "folder": "debugResources"
    }
  ]
}
```
For further information look at the example folder and the generated example_out.zip file.

### Building
For UNIX/Linux:

```
mkdir build && cd build
cmake ../
```


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* I took some inspiration from Qt's resource system and the qt resource compiler. If you want too learn more click [here](https://doc.qt.io/qt-5/resources.html) and [here](https://doc.qt.io/qt-5/rcc.html).
