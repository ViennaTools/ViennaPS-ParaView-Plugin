#!/usr/bin/env bash
#
# Launch the ViennaPS ParaView plugin container with X11 forwarding.
#
# Usage:
#   ./docker-run.sh                 # auto-detect GPU, launch ParaView GUI
#   USE_GPU=0 ./docker-run.sh       # force software rendering (no nvidia)
#   IMAGE=foo ./docker-run.sh       # override image tag
#   ./docker-run.sh --script=...    # pass extra args to paraview

set -euo pipefail

IMAGE="${IMAGE:-viennaps-paraview:latest}"
USE_GPU="${USE_GPU:-auto}"

GPU_ARGS=()
case "$USE_GPU" in
  auto)
    if command -v nvidia-smi >/dev/null 2>&1 \
       && docker info 2>/dev/null | grep -qi nvidia; then
      GPU_ARGS=(--gpus all)
      echo "[docker-run] NVIDIA runtime detected, enabling GPU passthrough."
    fi
    ;;
  1|true|yes)
    GPU_ARGS=(--gpus all)
    ;;
esac

if [[ -z "${DISPLAY:-}" ]]; then
  echo "[docker-run] DISPLAY is not set. Are you running in a graphical session?" >&2
  exit 1
fi

# Allow the container's root user to talk to the host X server.
if command -v xhost >/dev/null 2>&1; then
  xhost +local:root >/dev/null
  trap 'xhost -local:root >/dev/null 2>&1 || true' EXIT
else
  echo "[docker-run] 'xhost' not found; X11 forwarding may be rejected." >&2
fi

XAUTH_ARGS=()
if [[ -f "${HOME}/.Xauthority" ]]; then
  XAUTH_ARGS+=(--volume "${HOME}/.Xauthority:/root/.Xauthority:ro")
fi

exec docker run --rm -it \
  --env DISPLAY="${DISPLAY}" \
  --env QT_X11_NO_MITSHM=1 \
  --volume /tmp/.X11-unix:/tmp/.X11-unix:rw \
  "${XAUTH_ARGS[@]}" \
  --net=host \
  "${GPU_ARGS[@]}" \
  "${IMAGE}" "$@"
