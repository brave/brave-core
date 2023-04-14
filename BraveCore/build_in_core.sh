#!/bin/bash

set -e

current_arch=$(uname -m)
target_architecture="$current_arch"
current_dir="`pwd`/`dirname $0`"
framework_drop_point="$current_dir"
node_modules_path="$current_dir/../node_modules/brave-core-ios"

clean=0
build_simulator=0
build_device=0
release_flag="Release"
other_flags=""
brave_browser_dir="${@: -1}"

sim_dir="out/ios_Release_"$current_arch"_simulator"
device_dir="out/ios_Release_arm64"

function usage() {
  echo "Usage: ./build_in_core.sh [--clean] [--debug] {\$home/brave/brave-browser}"
  echo " --clean:           Cleans build directories before building"
  echo " --debug:           Builds a debug instead of release framework. (Should not be pushed with the repo)"
  echo " --build-simulator: Build only for simulator"
  echo " --build-device:    Build only for device"
  echo " --no-goma:         Builds without goma"
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
    if [ "$target_architecture" = "x86_64" ]; then
      sim_dir="out/ios_Debug_simulator"
    else
      sim_dir="out/ios_Debug_"$current_arch"_simulator"
    fi
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
    --no-goma)
    other_flags+=" --goma_offline"
    shift
    ;;
esac
done

# Fixing compiling //build/config/rust.gni on x86_64 which requires target_cpu="x64"
if [ "$target_architecture" = "x86_64" ]; then
  target_architecture="x64"
fi

# If neither argument is supplied, build both
if [ "$build_simulator" = 0 ] && [ "$build_device" = 0 ]; then
  build_simulator=1
  build_device=1
fi

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

if [ "$clean" = 1 ]; then
  # If this script has already been run, we'll clean out the build folders
  [[ -d $sim_dir ]] && gn clean $sim_dir
  [[ -d $device_dir ]] && gn clean $device_dir
else
  # Force it to reassemble the products if they've been built already
  # This prevents things that are only _copied_ into the directory over time in separate branches
  [[ -d $sim_dir/BraveCore.framework ]] && rm -rf $sim_dir/BraveCore.framework
  [[ -d $device_dir/BraveCore.framework ]] && rm -rf $device_dir/BraveCore.framework
fi

bc_framework_args=""
mc_framework_args=""

if [ "$build_simulator" = 1 ]; then
  npm run build -- $release_flag --target_os=ios --target_arch=$target_architecture --target_environment=simulator $other_flags
  bc_framework_args="-framework $sim_dir/BraveCore.framework"
  mc_framework_args="-framework $sim_dir/MaterialComponents.framework"
fi

if [ "$build_device" = 1 ]; then
  npm run build -- $release_flag --target_os=ios --target_arch=arm64 $other_flags
  bc_framework_args="$bc_framework_args -framework $device_dir/BraveCore.framework"
  mc_framework_args="$mc_framework_args -framework $device_dir/MaterialComponents.framework"
fi

[ -d "$framework_drop_point/BraveCore.xcframework" ] && rm -rf "$framework_drop_point/BraveCore.xcframework"
xcodebuild -create-xcframework $bc_framework_args -output "$framework_drop_point/BraveCore.xcframework"
echo "Created XCFramework: $framework_drop_point/BraveCore.xcframework"

[ -d "$framework_drop_point/MaterialComponents.xcframework" ] && rm -rf "$framework_drop_point/MaterialComponents.xcframework"
xcodebuild -create-xcframework $mc_framework_args -output "$framework_drop_point/MaterialComponents.xcframework"
echo "Created XCFramework: $framework_drop_point/MaterialComponents.xcframework"

echo "Moving Frameworks to node_modules"
mkdir -p "$node_modules_path"
rsync -a --delete "$framework_drop_point/BraveCore.xcframework" "$node_modules_path/"
rsync -a --delete "$framework_drop_point/MaterialComponents.xcframework" "$node_modules_path/"
echo "Moved Frameworks to node_modules"

cd brave
brave_core_build_hash=`git rev-parse HEAD`
brave_core_branch=`git symbolic-ref --short HEAD`
brave_core_tag=`git describe --tags --abbrev=0`

popd > /dev/null

echo "Completed building BraveCore from \`brave-core/$brave_core_build_hash\`"
cat > "$framework_drop_point/Local.resolved" << EOL
REMINDER: Your local brave-core-ios dependency has been overwritten in node_modules. Re-run bootstrap.sh when you are done testing.  

build: $release_flag
brave-browser: $brave_browser_branch ($brave_browser_build_hash)
brave-core: $brave_core_branch ($brave_core_build_hash)
  latest tag: $brave_core_tag

DO NOT COMMIT THIS FILE
EOL
