@ECHO OFF

ECHO "Building everything..."

PUSHD engine
CALL build.bat
POPD
IF %ERRORLEVEL% NEQ O (echo Error:%ERRORLEVEL% && exit)

PUSHD testbed
CALL bulid.bat
POPD
IF %ERRORLEVLE% NEQ 0 (echo Error:%ERRORLEVEL% && exit)

ECHO "Built all assemblies successfully"
