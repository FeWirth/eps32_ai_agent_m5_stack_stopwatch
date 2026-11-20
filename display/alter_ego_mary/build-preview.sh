#!/bin/bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
app=build/Alter-Ego-Mary.app
mkdir -p "$app/Contents/MacOS"
cat > "$app/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "https://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleExecutable</key><string>Alter-Ego-Mary</string>
  <key>CFBundleIdentifier</key><string>local.stopwatch.alter-ego-mary</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
clang++ -std=c++17 -O2 -Wall -Wextra -Werror -fobjc-arc -framework AppKit \
  -I ../../inspiration/hermes-gadget-sdk/firmware/core/include preview.mm \
  -o "$app/Contents/MacOS/Alter-Ego-Mary"
"$app/Contents/MacOS/Alter-Ego-Mary" "$PWD/build/preview.png"
