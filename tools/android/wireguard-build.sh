#!/bin/bash
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script is used to build the WireGuard tunnel library.
# This should be ran from the Brave root. aka `root/src/brave/`.
pushd .

# uses JAVA and SDK version from Chromium checkout.
# this syntax w/ realpath is expanding relative path to absolute path.
export JAVA_HOME=`realpath ../third_party/jdk/current`
export PATH=$PATH:$JAVA_HOME/bin
export ANDROID_HOME=`realpath ../third_party/android_sdk/public`
export PATH="$PATH:$ANDROID_HOME/cmdline-tools/latest/bin/"
echo "JAVA_HOME=$JAVA_HOME"
echo "ANDROID_HOME=$ANDROID_HOME"

BRAVE_DEPS_DIR=`realpath ./third_party/android_deps/libs/com_wireguard_android`
echo "Changing directory to $BRAVE_DEPS_DIR"
cd "$BRAVE_DEPS_DIR"
pushd .

echo "If you haven't accepted licenses yet this is interactive."
sdkmanager --licenses

WIREGUARD_TAG="1.0.20250531"
WIREGUARD_COMMIT="af29c672e78678aadf089b787188710bb9e6346d"
REPO_URL="https://git.zx2c4.com/wireguard-android"
echo "Cloning wireguard-android tag: $WIREGUARD_TAG with hash $WIREGUARD_COMMIT"
git clone --recurse-submodules --branch "$WIREGUARD_TAG" "$REPO_URL"
cd wireguard-android/

echo "Checking out hash: $WIREGUARD_COMMIT"
git reset --hard "$WIREGUARD_COMMIT"
./gradlew assembleRelease
popd

echo "Copying binary from tunnel/build/outputs/aar to expected Brave location"
COPY_SRC="wireguard-android/tunnel/build/outputs/aar/tunnel-release.aar"
COPY_DST="$BRAVE_DEPS_DIR/tunnel-$WIREGUARD_TAG.aar"
mv "$COPY_SRC" "$COPY_DST"

echo "Removing code that was cloned/built"
rm -rf wireguard-android/
popd

