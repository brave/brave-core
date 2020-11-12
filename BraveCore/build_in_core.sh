#!/bin/bash

set -e

current_dir="`pwd`/`dirname $0`"
framework_drop_point="$current_dir"
node_modules_path="$current_dir/../node_modules/brave-core-ios"

clean=0
build_simulator=0
build_device=0
release_flag="Release"
brave_browser_dir="${@: -1}"

base_path=""
sim_dir="out/ios_Release"
device_dir="out/ios_Release_arm64"

function usage() {
  echo "Usage: ./build_in_core.sh [--clean] [--debug] {\$home/brave/brave-browser}"
  echo " --clean:           Cleans build directories before building"
  echo " --debug:           Builds a debug instead of release framework. (Should not be pushed with the repo)"
  echo " --skip-update:     Skips cloning and rebasing"
  echo " --build-simulator: Build only for simulator"
  echo " --build-device:    Build only for device"
  exit 1
}

for i in "$@"
do
case $i in
    -h|--help)
    usage
    ;;
    --debug)
    release_flag="Debug"
    sim_dir="out/ios_Debug"
    device_dir="out/ios_Debug_arm64"
    shift
    ;;
    --clean)
    clean=1
    shift
    ;;
    --build-simulator)
    build_simulator=1
    shift
    ;;
    --build-device)
    build_device=1
    shift
    ;;
esac
done

if [ ! -d "$brave_browser_dir/src/brave" ]; then
  echo "Did not pass in a directory pointing to brave-browser which has already been init"
  echo "(by running \`npm run init\` at its root)"
  usage
fi

pushd $brave_browser_dir > /dev/null

brave_browser_build_hash=`git rev-parse HEAD`
brave_browser_branch=`git symbolic-ref --short HEAD`

# Do the rest of the work in the src folder
cd src

git fetch --tags --quiet

# [ -d brave/vendor/brave-rewards-ios ] && rm -r brave/vendor/brave-rewards-ios
# rsync -a --delete "$current_dir/../" brave/vendor/brave-rewards-ios

# TODO: Check if there are any changes made to any of the dependent vendors via git. If no files are changed,
#       we can just skip building altogether

if [ "$clean" = 1 ]; then
  # If this script has already been run, we'll clean out the build folders
  [[ -d $sim_dir ]] && gn clean  $sim_dir
  [[ -d $device_dir ]] && gn clean $device_dir
else
  # Force it to reassemble the products if they've been built already
  # This prevents things that are only _copied_ into the directory over time in separate branches
  [[ -d $sim_dir/BraveRewards.framework ]] && rm -rf $sim_dir/BraveRewards.framework
  [[ -d $device_dir/BraveRewards.framework ]] && rm -rf $device_dir/BraveRewards.framework
fi

if { [ "$build_simulator" = 1 ] && [ "$build_device" = 1 ]; } || { [ "$build_simulator" = 0 ] && [ "$build_device" = 0 ]; } ; then
  npm run build -- $release_flag --target_os=ios
  npm run build -- $release_flag --target_os=ios --target_arch=arm64
elif [ "$build_simulator" = 1 ]; then
  npm run build -- $release_flag --target_os=ios
elif [ "$build_device" = 1 ]; then
  npm run build -- $release_flag --target_os=ios --target_arch=arm64
fi

# Copy the framework structure (from iphoneos build) to the universal folder
if { [ "$build_simulator" = 1 ] && [ "$build_device" = 1 ]; } || { [ "$build_simulator" = 0 ] && [ "$build_device" = 0 ]; } || { [ "$build_simulator" = 0 ] && [ "$build_device" = 1 ]; } ; then
  base_path="$device_dir"
elif [ "$build_simulator" = 1 ]; then
  base_path="$sim_dir"
fi

rsync -a --delete "$base_path/BraveRewards.framework" "$framework_drop_point/"
rsync -a --delete "$base_path/MaterialComponents.framework" "$framework_drop_point/"

if [ -d "$base_path/BraveRewards.dSYM" ]; then
  # Copy the dSYM if available
  pushd $base_path > /dev/null
  # zip up the dSYM since its too big to upload
  zip -FSr "BraveRewards.dSYM.zip" "BraveRewards.dSYM"
  popd > /dev/null
  rsync -a --delete "$base_path/BraveRewards.dSYM.zip" "$framework_drop_point/"
fi

# Create universal binary file using lipo and place the combined executable in the copied framework directory
if { [ "$build_simulator" = 1 ] && [ "$build_device" = 1 ]; } || { [ "$build_simulator" = 0 ] && [ "$build_device" = 0 ]; } ; then
  lipo -create -output "$framework_drop_point/BraveRewards.framework/BraveRewards" "$sim_dir/BraveRewards.framework/BraveRewards" "$device_dir/BraveRewards.framework/BraveRewards"
  lipo -create -output "$framework_drop_point/MaterialComponents.framework/MaterialComponents" "$sim_dir/MaterialComponents.framework/MaterialComponents" "$device_dir/MaterialComponents.framework/MaterialComponents"
fi

echo "Created FAT framework: $framework_drop_point/BraveRewards.framework"

echo "Moving Frameworks to node_modules"
mkdir -p "$node_modules_path"
rsync -a --delete "$framework_drop_point/BraveRewards.framework" "$node_modules_path/"
rsync -a --delete "$framework_drop_point/MaterialComponents.framework" "$node_modules_path/"
rsync -a --delete "$framework_drop_point/BraveRewards.dSYM.zip" "$node_modules_path/"
rsync -a --delete "$framework_drop_point/copy_rewards_dsym.sh" "$node_modules_path/"
echo "Moved Frameworks to node_modules"

cd brave
brave_core_build_hash=`git rev-parse HEAD`
brave_core_branch=`git symbolic-ref --short HEAD`
brave_core_tag=`git describe --tags --abbrev=0`

popd > /dev/null

echo "Completed building BraveRewards from \`brave-core/$brave_core_build_hash\`"
cat > "$framework_drop_point/BraveRewards.resolved" << EOL
build: $release_flag
brave-browser: $brave_browser_branch ($brave_browser_build_hash)
brave-core: $brave_core_branch ($brave_core_build_hash)
  latest tag: $brave_core_tag
EOL

# Check if any of the includes had changed.
if `git diff --quiet "$node_modules_path/BraveRewards.framework/Headers/"`; then
  echo "  → No updates to library includes were made"
else
  echo "  → Changes found in library includes"
fi
