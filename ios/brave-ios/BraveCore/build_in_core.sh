#!/bin/bash

set -e

cd "${PROJECT_DIR}/../../.." # Back to brave-core root
target_environment=""
target_environment_dir=""
if [[ $PLATFORM_NAME == "iphonesimulator" ]]; then
  target_environment="--target_environment=simulator"
  target_environment_dir="_simulator"
fi
target_arch="arm64"
target_arch_dir="arm64"
if [[ $HOST_ARCH == "x86_64" ]]; then
  target_arch="x64"
  target_arch_dir=""
fi
npm run build -- $CONFIGURATION --target_os=ios --target_arch=$target_arch $target_environment
output_dir="iOS_${CONFIGURATION}_${target_arch_dir}${target_environment_dir}"

ios_dir="$(pwd)/ios/brave-ios/BraveCore/build"
cd ../out/$output_dir
rm -rf "$ios_dir/BraveCore.xcframework"
rm -rf "$ios_dir/MaterialComponents.xcframework"
xcodebuild -create-xcframework -framework "BraveCore.framework" -debug-symbols "$(pwd)/BraveCore.dSYM" -output "$ios_dir/BraveCore.xcframework"
xcodebuild -create-xcframework -framework "MaterialComponents.framework" -debug-symbols "$(pwd)/MaterialComponents.dSYM" -output "$ios_dir/MaterialComponents.xcframework"
