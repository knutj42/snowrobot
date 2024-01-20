mkdir -p build
pushd build
cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CC_COMPILER=gcc ..
cmake --build .
popd
