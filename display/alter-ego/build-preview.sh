#!/bin/bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
app=build/Alter-Ego.app
mkdir -p "$app/Contents/MacOS"
cat > "$app/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "https://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleExecutable</key><string>Alter-Ego</string>
  <key>CFBundleIdentifier</key><string>local.stopwatch.alter-ego</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
core=../../inspiration/hermes-gadget-sdk/firmware/core
clang++ -std=c++17 -O2 -Wall -Wextra -Werror -fobjc-arc -framework AppKit \
  -I "$core/include" preview.mm "$core/src/canvas.cpp" "$core/src/font5x7.cpp" \
  -o "$app/Contents/MacOS/Alter-Ego"
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/preview.png"
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/battery-preview.png" --battery
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/network-preview.png" --network
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/pomodoro-preview.png" --pomodoro
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/pomodoro-running-preview.png" --pomodoro-running
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/pomodoro-finished-preview.png" --pomodoro-finished
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/pomodoro-break-preview.png" --pomodoro-break
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/long-message.png" --long-message
"$app/Contents/MacOS/Alter-Ego" "$PWD/build/long-message-end.png" --long-message-end
