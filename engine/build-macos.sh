set echo on

mkdir -p ../bin

cFilenames=$(find . -type f -name "*.c")
assembly="engine"
compilerFlags="-g -shared -fdeclspec -fPIC"

includeFlags="-Isrc -I$VULKAN_SDK/include -I/Users/cluelssbutcher/libs/glfw-3.5.1.bin.MACOS/include"

linkerFlags="-lvulkan -lglfw.3 \
-L$VULKAN_SDK/lib \
-L/Users/cluelssbutcher/libs/glfw-3.5.1.bin.MACOS/lib-universal \
-framework Cocoa -framework IOKit -framework CoreFoundation \
-Wl,-rpath,$VULKAN_SDK/lib \
-Wl,-rpath,/Users/cluelssbutcher/libs/glfw-3.5.1.bin.MACOS/lib-universal"

defines="-D_DEBUG -DDEXPORT"

echo "Building $assembly..."
clang $cFilenames $compilerFlags -o ../bin/lib$assembly.dylib $defines $includeFlags $linkerFlags