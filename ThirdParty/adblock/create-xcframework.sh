#!/bin/bash

set -e

xcodebuild archive \
-scheme AdblockRust \
-destination "generic/platform=iOS" \
-archivePath "./build/AdblockRust-iOS"

xcodebuild archive \
-scheme AdblockRust \
-destination "generic/platform=iOS Simulator" \
-archivePath "./build/AdblockRust-Simulator"

if [ -d AdblockRust.xcframework ]; then
  rm -r AdblockRust.xcframework
fi

xcodebuild -create-xcframework \
-framework "./build/AdblockRust-iOS.xcarchive/Products/Library/Frameworks/AdblockRust.framework" \
-debug-symbols "$(pwd)/build/AdblockRust-iOS.xcarchive/dSYMs/AdblockRust.framework.dSYM" \
-framework "./build/AdblockRust-Simulator.xcarchive/Products/Library/Frameworks/AdblockRust.framework" \
-debug-symbols "$(pwd)/build/AdblockRust-Simulator.xcarchive/dSYMs/AdblockRust.framework.dSYM" \
-output AdblockRust.xcframework
