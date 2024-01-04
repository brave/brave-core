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

ios_dir="$(pwd)/ios/brave-ios/BraveCore/build"
cp -fR "$ios_dir/../placeholders/BraveCore.xcframework" "$ios_dir"
cp -fR "$ios_dir/../placeholders/MaterialComponents.xcframework" "$ios_dir"

npm run build -- $CONFIGURATION --target_os=ios --target_arch=$target_arch $target_environment
output_dir="iOS_${CONFIGURATION}_${target_arch_dir}${target_environment_dir}"

cd ../out/$output_dir
rm -rf "$ios_dir/BraveCore.xcframework"
rm -rf "$ios_dir/MaterialComponents.xcframework"
xcodebuild -create-xcframework -framework "BraveCore.framework" -debug-symbols "$(pwd)/BraveCore.dSYM" -output "$ios_dir/BraveCore.xcframework"
xcodebuild -create-xcframework -framework "MaterialComponents.framework" -debug-symbols "$(pwd)/MaterialComponents.dSYM" -output "$ios_dir/MaterialComponents.xcframework"

# Delete Chromium Assets from BraveCore.framework since they aren't used.
# TODO: Get this removed in the brave-core builds if possible
find "$ios_dir/BraveCore.xcframework" -name 'BraveCore.framework' -print0 | while read -d $'\0' framework
do
  if [[ -f "$framework/Assets.car" ]]; then
    rm "$framework/Assets.car"
  fi
done

# Makes an xcconfig file with some GN args such as versioning and api keys
copy_args=(
  "brave_version_major"
  "brave_version_minor"
  "brave_version_patch"
  "brave_version_build"
  "brave_services_key"
  "brave_stats_api_key"
)
args_file="$ios_dir/args.xcconfig"
if [ -f $args_file ]; then
  rm $args_file
fi
for key in "${copy_args[@]}"; do
  matched_line=$(grep "^$key = " "args.gn") || true
  if [ -n "$matched_line" ]; then
      echo "$matched_line" | sed 's/"//g' >> "$args_file"
  fi
done
