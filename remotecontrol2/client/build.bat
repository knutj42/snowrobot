if not exist build mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CC_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=mingw32-make -G "MinGW Makefiles" .
cmake --build .
cd ..