# ViennaPS ParaView Plugin

A ParaView plugin for the [ViennaPS](https://github.com/ViennaTools/ViennaPS) semiconductor process simulation library. Adds a *ViennaPS Geometry* source and a *ViennaPS Process* filter to ParaView, so you can build domains and run process simulations interactively.


## Dependencies

- ParaView 6.1 (built from source, with Qt6 and Python)
- ViennaPS 4.7.0, ViennaCore 2.2.1, ViennaLS 5.8.5, ViennaRay 4.3.1
- ViennaCS 2.1.2, ViennaHRLE 1.1.2, Embree 4
- C++20 compiler, CMake, OpenMP

`CMakeLists.txt` expects ParaView and the Vienna libraries to be reachable via CMake's find mechanism. Point it to your local installs through the variables below.

## Build

```bash
mkdir -p build && cd build
cmake .. -DParaView_DIR=/path/to/paraview/build -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The plugin lands in `build/lib/paraview-6.1/plugins/ViennaPSPluginModule/ViennaPSPluginModule.so`.

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

## Docker (GUI via X11)

A `Dockerfile` is provided so reviewers can run the plugin without building ParaView themselves. The image bundles ParaView 6.1.0 (Qt6 + Python) and the plugin; the GUI is exposed to the host through X11 forwarding.

### Build the image

```bash
docker build -t viennaps-paraview .
```

First build downloads ParaView + submodules and compiles everything; expect 30–60 min depending on your machine. Subsequent builds reuse layers.

To pin a different ParaView ref (e.g. a release candidate or newer version):

```bash
docker build --build-arg PARAVIEW_TAG=v6.1.0-RC2 -t viennaps-paraview .
```

### Run the GUI

```bash
./docker-run.sh
```

The helper script handles `xhost`, mounts the X11 socket and `~/.Xauthority`, and passes `--gpus all` when an NVIDIA runtime is present. The plugin auto-loads via `PV_PLUGIN_PATH`, no manual *Load New…* step needed.

Tested on Windows (WSL2) through WSLg, where the GUI works out of the box.
