#!/bin/bash

set -e

cd $PROJECT_DIR
src_dir="$PROJECT_DIR/../../../.."
echo "Current Dir: $(pwd)"

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

output_dir="$src_dir/out/ios_Build"
mkdir -p $output_dir

cp -fR "$(pwd)/../BraveCore/build/placeholders/BraveCore.xcframework" "$output_dir"
cp -fR "$(pwd)/../BraveCore/build/placeholders/MaterialComponents.xcframework" "$output_dir"

npm run build -- $CONFIGURATION --target_os=ios --target_arch=$target_arch $target_environment

# Create xcframeworks
pushd "$src_dir/out/iOS_${CONFIGURATION}_${target_arch_dir}${target_environment_dir}"
rm -rf "$output_dir/BraveCore.xcframework"
xcodebuild -create-xcframework -framework "BraveCore.framework" -debug-symbols "$(pwd)/BraveCore.dSYM" -output "$output_dir/BraveCore.xcframework"
rm -rf "$output_dir/MaterialComponents.xcframework"
xcodebuild -create-xcframework -framework "MaterialComponents.framework" -debug-symbols "$(pwd)/MaterialComponents.dSYM" -output "$output_dir/MaterialComponents.xcframework"

# Delete Chromium Assets from BraveCore.framework since they aren't used.
# TODO(@brave/ios): Get this removed in the brave-core builds if possible
find "$output_dir/BraveCore.xcframework" -name 'BraveCore.framework' -print0 | while read -d $'\0' framework
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
args_file="$output_dir/args.xcconfig"
if [ -f $args_file ]; then
  rm $args_file
fi
for key in "${copy_args[@]}"; do
  matched_line=$(grep "^$key = " "args.gn") || true
  if [ -n "$matched_line" ]; then
      echo "$matched_line" | sed 's/"//g' >> "$args_file"
  fi
done

popd
