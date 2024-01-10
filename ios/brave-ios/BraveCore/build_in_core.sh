#!/bin/bash

# TODO(@brave/ios): Move contents of this into `npm run build` command

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

# Update symlink
npm run update_symlink -- $CONFIGURATION --symlink_dir "$src_dir/out/current_link" --target_os=ios --target_arch=$target_arch $target_environment

output_dir="$src_dir/out/current_link"

cp -fR "$(pwd)/../BraveCore/placeholders/BraveCore.xcframework" "$output_dir"
cp -fR "$(pwd)/../BraveCore/placeholders/MaterialComponents.xcframework" "$output_dir"

npm run build -- $CONFIGURATION --target_os=ios --target_arch=$target_arch $target_environment

# Create xcframeworks
npm run ios_create_xcframeworks -- $CONFIGURATION --target_arch=$target_arch $target_environment

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
  matched_line=$(grep "^$key = " "$output_dir/args.gn") || true
  if [ -n "$matched_line" ]; then
      echo "$matched_line" | sed 's/"//g' >> "$args_file"
  fi
done
