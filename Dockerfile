# syntax=docker/dockerfile:1.6

ARG UBUNTU_VERSION=24.04
ARG PARAVIEW_TAG=v6.1.0
ARG BUILD_JOBS=4

# Stage 1: dev — ParaView from source + build tooling (published as :dev, reused by CI).
FROM ubuntu:${UBUNTU_VERSION} AS dev

ARG PARAVIEW_TAG
ARG BUILD_JOBS
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
      git \
      ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
RUN git clone --depth 1 --branch ${PARAVIEW_TAG} \
      --recurse-submodules --shallow-submodules \
      https://gitlab.kitware.com/paraview/paraview.git

RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ninja-build \
      pkg-config \
      python3 \
      python3-dev \
      python3-numpy \
      libomp-dev \
      libtbb-dev \
      qt6-base-dev \
      qt6-tools-dev \
      qt6-tools-dev-tools \
      libqt6opengl6-dev \
      libqt6svg6-dev \
      libqt6core5compat6-dev \
      libgl1-mesa-dev \
      libglu1-mesa-dev \
      libxt-dev \
      libxext-dev \
      libxrender-dev \
      libxi-dev \
      libxmu-dev \
      libxcursor-dev \
      libxrandr-dev \
      libxinerama-dev \
      libxkbcommon-dev \
      libfontconfig1-dev \
      libfreetype-dev \
      libxml2-dev \
      xsltproc \
      zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src/paraview/build
RUN cmake -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/paraview \
      -DPARAVIEW_USE_QT=ON \
      -DPARAVIEW_USE_PYTHON=ON \
      -DPARAVIEW_USE_QTWEBENGINE=OFF \
      -DPARAVIEW_BUILD_TESTING=OFF \
      -DPARAVIEW_BUILD_EXAMPLES=OFF \
      -DPARAVIEW_BUILD_DEVELOPER_DOCUMENTATION=OFF \
      .. \
 && ninja -j${BUILD_JOBS} \
 && ninja install \
 && rm -rf /src/paraview

# Prebuilt so ViennaRay's CPMFindPackage reuses it via find_package() instead of
# recompiling. Options match ViennaRay's defaults (v4.3.3, ray masking on).
ARG EMBREE_TAG=v4.3.3
RUN git clone --depth 1 --branch ${EMBREE_TAG} \
      https://github.com/embree/embree.git /src/embree \
 && cmake -GNinja -S /src/embree -B /src/embree/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/embree \
      -DEMBREE_RAY_MASK=ON \
      -DEMBREE_ISPC_SUPPORT=OFF \
      -DEMBREE_TUTORIALS=OFF \
 && ninja -C /src/embree/build -j${BUILD_JOBS} \
 && ninja -C /src/embree/build install \
 && rm -rf /src/embree
# Make the prebuilt Embree discoverable by find_package() in the plugin/ViennaRay build.
ENV CMAKE_PREFIX_PATH=/opt/embree

# Stage 2: builder — builds the plugin on top of dev.
FROM dev AS builder

ARG BUILD_JOBS

WORKDIR /src/plugin
COPY CMakeLists.txt ./
COPY cmake ./cmake
COPY Plugin ./Plugin

WORKDIR /src/plugin/build
RUN cmake -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DParaView_DIR=/opt/paraview/lib/cmake/paraview-6.1 \
      .. \
 && ninja -j${BUILD_JOBS} \
 && mkdir -p /opt/plugin \
 && cp -r lib/paraview-6.1/plugins/. /opt/plugin/ \
 && find /opt/embree -name "libembree*.so*" -exec cp -aP {} /opt/paraview/lib/ \;

# Stage 3: runtime — slim image with ParaView + plugin, no build tooling.
FROM ubuntu:${UBUNTU_VERSION} AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
      libqt6core6 \
      libqt6gui6 \
      libqt6widgets6 \
      libqt6opengl6 \
      libqt6openglwidgets6 \
      libqt6svg6 \
      libqt6core5compat6 \
      libqt6network6 \
      libqt6dbus6 \
      libqt6sql6 \
      libqt6xml6 \
      libqt6help6 \
      libqt6printsupport6 \
      libqt6concurrent6 \
      libqt6uitools6 \
      libgl1 \
      libglx-mesa0 \
      libegl1 \
      libglu1-mesa \
      libxt6 \
      libxext6 \
      libxrender1 \
      libxi6 \
      libxmu6 \
      libxcursor1 \
      libxrandr2 \
      libxinerama1 \
      libfontconfig1 \
      libfreetype6 \
      libxkbcommon0 \
      libxkbcommon-x11-0 \
      libxcb-icccm4 \
      libxcb-image0 \
      libxcb-keysyms1 \
      libxcb-randr0 \
      libxcb-render-util0 \
      libxcb-shape0 \
      libxcb-sync1 \
      libxcb-xfixes0 \
      libxcb-xkb1 \
      libxcb-cursor0 \
      libdbus-1-3 \
      libomp5 \
      libtbb12 \
      python3 \
      python3-numpy \
      libpython3.12 \
      xauth \
      ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/paraview /opt/paraview
COPY --from=builder /opt/plugin   /opt/paraview/lib/paraview-6.1/plugins

ENV PATH=/opt/paraview/bin:${PATH}
ENV LD_LIBRARY_PATH=/opt/paraview/lib:${LD_LIBRARY_PATH}
ENV PV_PLUGIN_PATH=/opt/paraview/lib/paraview-6.1/plugins/ViennaPSPluginModule
ENV QT_X11_NO_MITSHM=1

ENTRYPOINT ["/opt/paraview/bin/paraview"]
