#!/bin/bash

set -e

xcodebuild archive \
-scheme sqlcipher \
-destination "generic/platform=iOS" \
-archivePath "./build/sqlcipher-iOS"

xcodebuild archive \
-scheme sqlcipher \
-destination "generic/platform=iOS Simulator" \
-archivePath "./build/sqlcipher-Simulator"

if [ -d sqlcipher.xcframework ]; then
  rm -r sqlcipher.xcframework
fi

xcodebuild -create-xcframework \
-framework "./build/sqlcipher-iOS.xcarchive/Products/Library/Frameworks/sqlcipher.framework" \
-debug-symbols "$(pwd)/build/sqlcipher-iOS.xcarchive/dSYMs/sqlcipher.framework.dSYM" \
-framework "./build/sqlcipher-Simulator.xcarchive/Products/Library/Frameworks/sqlcipher.framework" \
-debug-symbols "$(pwd)/build/sqlcipher-Simulator.xcarchive/dSYMs/sqlcipher.framework.dSYM" \
-output sqlcipher.xcframework
