    set -euo pipefail

mkdir -p ../bin

VULKAN="/usr/local/opt/vulkan-loader"
MOLTENVK="/usr/local/opt/molten-vk"
GLFW="/usr/local/opt/glfw"
HEADERS="/usr/local/opt/vulkan-headers"

C_FILES=$(find src -type f -name "*.c")

echo "Building engine..."

clang $C_FILES \
    -g -dynamiclib -fdeclspec -fPIC \
    -D_DEBUG -DDEXPORT \
    -Isrc \
    -I"$HEADERS/include" \
    -I"$GLFW/include" \
    -L"$VULKAN/lib" \
    -L"$MOLTENVK/lib" \
    -L"$GLFW/lib" \
    -lvulkan \
    -lglfw \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation \
    -o ../bin/libengine.dylib \
    -Wl,-rpath,"$VULKAN/lib" \
    -Wl,-rpath,"$MOLTENVK/lib" \
    -Wl,-rpath,"$GLFW/lib"

echo "Engine built successfully."
