# ViennaPS ParaView Plugin

A ParaView plugin for the [ViennaPS](https://github.com/ViennaTools/ViennaPS) semiconductor process simulation library. Adds a *ViennaPS Geometry* source and a *ViennaPS Process* filter to ParaView, so you can build domains and run process simulations interactively.

This is the practical part of my Bachelor's thesis at TU Wien.

## Dependencies

- ParaView 5.13 (built from source, with Qt6)
- ViennaPS 4.4.0, ViennaCore 2.1.2, ViennaLS 5.7.1, ViennaRay 4.1.2
- ViennaCS 2.0.1, ViennaHRLE 1.0.0, Embree 4
- C++20 compiler, CMake, OpenMP

The paths in `CMakeLists.txt` and `build.sh` assume ParaView and the Vienna libraries live under `~/BachelorThesis/`. Adjust if needed.

## Build

```bash
./build.sh
```

The plugin lands in `build/lib/paraview-5.13/plugins/ViennaPSPluginModule/ViennaPSPluginModule.so`.

## Run

```bash
./test.sh
```

This launches ParaView with the plugin already loaded. To load it manually instead, open ParaView and go to *Tools → Manage Plugins → Load New…* and pick the `.so` file above.
