set echo on

echo "Building everything..."

make -f Makefile.engine.linux.mak all

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
    echo "Error:"$ERRORLEVEL && exit
fi

make -f Makefile.testbed.linux.mak all  

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
    echo "Error:"$ERRORLEVEL && exit
fi

make -f Makefile.tests.linux.mak all
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
    echo "Error:"$ERRORLEVEL && exit
fi

echo "Built all assemblies successfully"
