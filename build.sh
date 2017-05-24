set -e

BUILD_TYPE="Debug"

mkdir -p $BUILD_TYPE
cd $BUILD_TYPE
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j

cp client/client ../client_upload
cp server/server ../server_upload
