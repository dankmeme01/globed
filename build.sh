export SPLAT_DIR=/home/dankpc/programming/geode/xwin-splat
export TOOLCHAIN_REPO=/home/dankpc/programming/geode/clang-msvc-sdk

export HOST_ARCH=x86
export MSVC_BASE=$SPLAT_DIR/crt
export WINSDK_BASE=$SPLAT_DIR/sdk
# TODO: get this from the thing..
export WINSDK_VER=10.0.20348
# change this to your llvm version!!!
export LLVM_VER=16
export CLANG_VER=$LLVM_VER

echo running initial cmake
cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_REPO/clang-cl-msvc.cmake \
  -DCMAKE_BUILD_TYPE=Release -DGEODE_DISABLE_FMT_CONSTEVAL=1 -B build
  
echo running cmake build
cmake --build build --config Release
