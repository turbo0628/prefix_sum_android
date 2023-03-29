# This script is run on Android via `adb shell`.
cd /data/local/tmp/prefix_sum
chmod 755 ./TaichiAot
chmod 755 ./libtaichi_c_api.so
export LD_LIBRARY_PATH="/data/local/tmp/prefix_sum"
./TaichiAot
