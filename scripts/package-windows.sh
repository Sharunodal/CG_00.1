#!/usr/bin/env bash
# Package Windows files for the game: copies exe, SDL3.dll (or SDL2.dll), assets and shaders into dist/windows
# Usage: scripts/package-windows.sh [--exe EXE] [--sdl-libdir DIR] [--out DIR] [--nozip]
# Defaults:
#   EXE: ./test.exe
#   SDL_LIBDIR: src/thirdparty/SDL3/lib
#   OUT: dist/windows
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." >/dev/null && pwd)

# Defaults
EXE="${REPO_ROOT}/test.exe"
SDL_LIBDIR="${REPO_ROOT}/src/thirdparty/SDL3/lib"
OUT_DIR="${REPO_ROOT}/dist/windows"
ZIP=true
COPY_ASSETS=true

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    --exe) EXE="$2"; shift 2;;
    --sdl-libdir) SDL_LIBDIR="$2"; shift 2;;
    --out) OUT_DIR="$2"; shift 2;;
    --nozip) ZIP=false; shift 1;;
    --noassets) COPY_ASSETS=false; shift 1;;
    --help) echo "Usage: $0 [--exe PATH] [--sdl-libdir DIR] [--out DIR] [--nozip] [--noassets]"; exit 0;;
    *) echo "Unknown arg: $1"; exit 2;;
  esac
done

echo "Packaging Windows build"
echo "EXE: ${EXE}"
echo "SDL libdir: ${SDL_LIBDIR}"
echo "Output: ${OUT_DIR}"

# Basic checks
if [ ! -f "${EXE}" ]; then
  echo "ERROR: EXE not found at ${EXE}"
  exit 1
fi

# Create output dir
mkdir -p "${OUT_DIR}"

# Copy binary
cp -f "${EXE}" "${OUT_DIR}/" || true

# Copy assets and shaders
if [ "$COPY_ASSETS" = true ]; then
  if [ -d "${REPO_ROOT}/assets" ]; then
    cp -r "${REPO_ROOT}/assets" "${OUT_DIR}/" || true
  else
    echo "Warning: assets/ not found in repo root; skipping copying assets"
  fi
  if [ -d "${REPO_ROOT}/src/shaders" ]; then
    mkdir -p "${OUT_DIR}/src"
    cp -r "${REPO_ROOT}/src/shaders" "${OUT_DIR}/src/shaders" || true
  else
    echo "Warning: src/shaders/ not found; skipping"
  fi
fi

# Copy SDL runtime DLL
# Priority: WIN_SDL_DLL (env), SDL3.dll or SDL2.dll under SDL_LIBDIR, then search src/thirdparty
if [ -n "${WIN_SDL_DLL:-}" ] && [ -f "${WIN_SDL_DLL}" ]; then
  echo "Using SDL DLL from WIN_SDL_DLL=${WIN_SDL_DLL}"
  cp -f "${WIN_SDL_DLL}" "${OUT_DIR}/" || true
else
  SDL_DLL=""
  if [ -d "${SDL_LIBDIR}" ]; then
    # Check common locations in SDL_LIBDIR sibling folders
    for candidate in "$SDL_LIBDIR/SDL3.dll" "$SDL_LIBDIR/SDL2.dll" "$SDL_LIBDIR"/*.dll "$SDL_LIBDIR/../bin/SDL3.dll" "$SDL_LIBDIR/../bin/SDL2.dll"; do
      if [ -f "$candidate" ]; then
        SDL_DLL="$candidate"
        break
      fi
    done
  fi
  if [ -z "$SDL_DLL" ]; then
    # Try a broader search inside src/thirdparty
    FOUND_DLL=$(find "${REPO_ROOT}/src/thirdparty" -type f \( -iname 'SDL3.dll' -o -iname 'SDL2.dll' \) -print -quit || true)
    if [ -n "$FOUND_DLL" ]; then
      SDL_DLL="$FOUND_DLL"
    fi
  fi

  if [ -n "$SDL_DLL" ]; then
    echo "Copying SDL runtime: ${SDL_DLL}"
    cp -f "${SDL_DLL}" "${OUT_DIR}/" || true
  else
    echo "Warning: SDL runtime DLL not found; you must provide SDL3.dll or SDL2.dll next to the exe on Windows"
  fi
fi

# README
cat > "${OUT_DIR}/README_WINDOWS.txt" <<EOF
This folder contains a Windows build of the game.

How to run:
 - Make sure you have the SDL runtime DLL (SDL3.dll or SDL2.dll) in the same folder as the executable.
 - Run the executable: $(basename "${EXE}")

If you built this with cross-compile on Linux, copy the Windows SDL runtime into this folder or let the script find it under src/thirdparty.
EOF

# Zip
if [ "$ZIP" = true ]; then
  echo "Creating zip archive..."
  mkdir -p "${REPO_ROOT}/dist"
  (cd "${OUT_DIR}" && zip -r "../../dist/$(basename ${REPO_ROOT})-windows.zip" .) >/dev/null 2>&1 || true
  echo "Created dist/$(basename ${REPO_ROOT})-windows.zip"
fi

echo "Packaging complete: ${OUT_DIR}"
exit 0
