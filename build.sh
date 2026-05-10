#!/bin/bash
set -e

PARAVIEW_DIR="~/BachelorThesis/paraview/build"

mkdir -p build
cd build
rm -rf *

cmake .. -DParaView_DIR=${PARAVIEW_DIR} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
make -j16

if [ -f *.so ]; then
    ls -la *.so
fi
