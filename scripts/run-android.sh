#!/bin/bash
set -e

rm -rf build-android-aarch64
mkdir build-android-aarch64
pushd build-android-aarch64
TAICHI_C_API_INSTALL_DIR="${PWD}/../build-taichi-android-aarch64/install/c_api" cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_ABI="arm64-v8a" \
    -G "Ninja"
if [ $? -ne 0 ]; then
    echo "Configuration failed"
    exit -1
fi

cmake --build .
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit -1
fi
popd

#if [[ ! -f "./prefix_sum_module/metadata.tcb" ]]; then
#    echo "Did you run python app.py --arch=vulkan?"
#    exit -1
#fi

adb shell rm -rf /data/local/tmp/prefix_sum/
adb shell mkdir /data/local/tmp/prefix_sum/
adb push ./build-android-aarch64/TaichiAot /data/local/tmp/prefix_sum/
adb push ./build-android-aarch64/libtaichi_c_api.so /data/local/tmp/prefix_sum/
adb push ./prefix_sum_module /data/local/tmp/prefix_sum/
adb push ./scripts/__android_main.sh /data/local/tmp/prefix_sum/
adb shell chmod 755 /data/local/tmp/prefix_sum/__android_main.sh
adb shell sh /data/local/tmp/prefix_sum/__android_main.sh
