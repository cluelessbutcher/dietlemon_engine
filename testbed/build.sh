set echo on

mkdir -p ../bin

cFilenames=$(find . -type f -name "*.c")

assembly="testbed"
compilerFlags="-g -fdeclspec -fPIC"
includeFlags="-Isrc -I../engine/src/"
linkerFlags="-L../bin/ -lengine -Wl, -rpath ."
defines="-D_DEBUG -DDIMPORT"

echo "Building %assembly"
echo clang $cFilenames $compilerFlags -o ../bin/$assembly $defines $includeFlags $linkerFlags
clang $cFilenames $compilerFlags -o ../bin/$assembly $defines $includeFlags $linkerFlags
