# ViennaPS ParaView Plugin

A ParaView plugin for the [ViennaPS](https://github.com/ViennaTools/ViennaPS) semiconductor process simulation library. Adds a *ViennaPS Geometry* source and a *ViennaPS Process* filter to ParaView, so you can build domains and run process simulations interactively.


## Dependencies

- ParaView 5.13 (built from source, with Qt6)
- ViennaPS 4.4.0, ViennaCore 2.1.2, ViennaLS 5.7.1, ViennaRay 4.1.2
- ViennaCS 2.0.1, ViennaHRLE 1.0.0, Embree 4
- C++20 compiler, CMake, OpenMP

`CMakeLists.txt` expects ParaView and the Vienna libraries to be reachable via CMake's find mechanism. Point it to your local installs through the variables below.

## Build

```bash
mkdir -p build && cd build
cmake .. -DParaView_DIR=/path/to/paraview/build -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The plugin lands in `build/lib/paraview-5.13/plugins/ViennaPSPluginModule/ViennaPSPluginModule.so`.

## Run

Open ParaView and load the plugin via *Tools → Manage Plugins → Load New…*, then pick the `.so` file above.

To start ParaView with the plugin already loaded, drop this into a Python file:

```python
from paraview.simple import *
LoadPlugin('/absolute/path/to/ViennaPSPluginModule.so')
```

and launch ParaView with it:

```bash
paraview --script=load_plugin.py
```

For debugging, prepend `gdb --args` or run under `valgrind` (use ParaView's `vtkkwiml.supp` suppression file to cut noise).
