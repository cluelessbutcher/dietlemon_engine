
#!/bin/bash
set -euo pipefail

mkdir -p ../bin

VULKAN="/usr/local/opt/vulkan-loader"
GLFW="/usr/local/opt/glfw"
HEADERS="/usr/local/opt/vulkan-headers"

cFilenames=$(find src -type f -name "*.c")

echo "Building engine..."

clang $cFilenames \
    -g -dynamiclib -fdeclspec -fPIC \
    -D_DEBUG -DDEXPORT \
    -Isrc \
    -I"$HEADERS/include" \
    -I"$GLFW/include" \
    -L"$VULKAN" \
    -L"$GLFW/lib" \
    -lvulkan -lglfw \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation \
    -o ../bin/libengine.dylib \
    -Wl,-rpath,"$VULKAN/lib" \
    -Wl,-rpath,"$GLFW/lib"

echo "Engine built successfully."
