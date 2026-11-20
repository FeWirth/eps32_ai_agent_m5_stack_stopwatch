#!/bin/bash
set -euo pipefail
: "${IDF_PATH:?Activate ESP-IDF before building the StopWatch firmware}"
art_dir="$(cd -- "$(dirname -- "$0")" && pwd)"
sdk="$art_dir/../../inspiration/hermes-gadget-sdk"
build="$art_dir/build/esp32"
mkdir -p "$build" "$art_dir/build/component-cache"
export PYTHONPATH="$art_dir/build/local-python${PYTHONPATH:+:$PYTHONPATH}"
export IDF_COMPONENT_CACHE_PATH="$art_dir/build/component-cache"
cd "$sdk/firmware/esp32"
python3 "$IDF_PATH/tools/idf.py" -B "$build" -DIDF_TARGET=esp32s3 -DSDKCONFIG="$build/sdkconfig" \
  '-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;boards/m5stack-stopwatch/sdkconfig.defaults' \
  -DHG_ALTER_EGO_DIR="$art_dir" build
