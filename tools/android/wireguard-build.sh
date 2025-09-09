#!/usr/bin/env bash

# Script used to build the WireGuard tunnel library.
# This should be ran from the Brave root. aka `root/src/brave/`.
pushd .

# uses JAVA and SDK version from Chromium checkout.
# this syntax w/ pwd is expanding relative path to absolute path.
export JAVA_HOME=`realpath ../third_party/jdk/current`
export PATH=$PATH:$JAVA_HOME/bin
export ANDROID_HOME=`realpath ../third_party/android_sdk/public`
export PATH="$PATH:$ANDROID_HOME/cmdline-tools/latest/bin/"
echo "JAVA_HOME=$JAVA_HOME"
echo "ANDROID_HOME=$ANDROID_HOME"

BRAVE_ANDROID_DEPS_DIR=`realpath ./third_party/android_deps/libs/com_wireguard_android`
echo "Changing directory to $BRAVE_ANDROID_DEPS_DIR"
cd "$BRAVE_ANDROID_DEPS_DIR"
pushd .

echo "Accepting licenses. If this is a first time, you will need to type Y/N and hit enter for each"
sdkmanager --licenses

WIREGUARD_TAG="1.0.20250531"
echo "Cloning wireguard-android tag: $WIREGUARD_TAG"
git clone --recurse-submodules --branch "$WIREGUARD_TAG" https://git.zx2c4.com/wireguard-android
cd wireguard-android/

echo "Building wireguard-around tag: $WIREGUARD_TAG"
./gradlew assembleRelease
popd

echo "Copying binary from tunnel/build/outputs/aar to expected Brave location"
mv wireguard-android/tunnel/build/outputs/aar/tunnel-release.aar "$BRAVE_ANDROID_DEPS_DIR/tunnel-$WIREGUARD_TAG.aar"

echo "Removing code that was cloned/built"
rm -rf wireguard-android/
popd

