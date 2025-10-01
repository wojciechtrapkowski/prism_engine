#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status

# Default build type
BUILD_TYPE="Debug"

# Parse command line arguments
REBUILD=0
for arg in "$@"; do
  case $arg in
    --rebuild)
      REBUILD=1
      shift
      ;;
    --debug)
      BUILD_TYPE="Debug"
      shift
      ;;
    --release)
      BUILD_TYPE="Release"
      shift
      ;;
    *)
      # Unknown option
      ;;
  esac
done

BUILD_DIR="build/$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"

# Check if the build directory exists
if [ ! -d "$BUILD_DIR" ]; then
  echo "[WARNING] Build directory $BUILD_DIR not found."
  ./build.sh --build-type $BUILD_TYPE
fi

# Check if we need to build
if [ ! -f "$BUILD_DIR/bin/PrismMain" ] || [ $REBUILD -eq 1 ]; then
  echo "[INFO] Building project ($BUILD_TYPE)..."
  if ! ./build.sh --build-type $BUILD_TYPE; then
    echo "[ERROR] Build failed."
    exit 1
  fi
fi

# Navigate to the build bin directory
cd "$BUILD_DIR/bin"

# Check if the binary exists
if [ -f "PrismMain" ]; then
  echo "[INFO] Running Prism Engine ($BUILD_TYPE)..."
  ./PrismMain
else
  echo "[ERROR] PrismMain executable not found in $BUILD_DIR/bin. Please check build errors."
  exit 1
fi

# Return to the project root
cd ../../..
echo "[INFO] Application terminated."