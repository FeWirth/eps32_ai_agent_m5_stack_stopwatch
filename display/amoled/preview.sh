#!/bin/bash
set -euo pipefail

cd -- "$(dirname -- "$0")"
sdk=../../inspiration/hermes-gadget-sdk
python3 prepare_art.py
clang++ -std=c++17 -Wall -Wextra -Werror -I "$sdk/firmware/core/include" \
  -I build preview.cpp -o build/preview
build/preview build/preview.ppm
sips -s format png build/preview.ppm --out build/preview.png
