
    set -euo pipefail

mkdir -p ../bin

VULKAN="/usr/local"
MOLTENVK="/usr/local"
GLFW="/usr/local/opt/glfw"
HEADERS="$HOME/VulkanSDK/1.4.363.1/macOS/include"

C_FILES=$(find src -type f -name "*.c")

echo "Building engine..."

clang $C_FILES \
    -g -dynamiclib -fdeclspec -fPIC \
    -D_DEBUG -DDEXPORT \
    -Isrc \
    -I"$HEADERS" \
    -I"$GLFW/include" \
    -L"$VULKAN/lib" \
    -L"$MOLTENVK/lib" \
    -L"$GLFW/lib" \
    -lvulkan \
    -lglfw \
    -lMoltenVK \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation \
    -o ../bin/libengine.dylib \
    -Wl,-rpath,/usr/local/lib \
    -Wl,-rpath,"$GLFW/lib"

echo "Engine built successfully."
