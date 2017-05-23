set -e

BUILD_TYPE="Debug"

mkdir -p $BUILD_TYPE
cd $BUILD_TYPE
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j

cp client/client ../clt
cp server/server ../svr
