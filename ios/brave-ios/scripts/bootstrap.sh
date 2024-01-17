#!/bin/sh
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

set -e

this_dir=$(dirname $0)
src_dir=$(realpath "$this_dir/../../../..")

# Initial packing of JS so SPM doesnt show warnings that the files are missing
# Subsequent builds will run this to ensure those files are kept up to date
npm run ios_pack_js

# Set up BraveCore placeholders to allow SPM to validate the package
# This folder will be replaced on first build to a symlink to the current
# build config
build_output_dir="$src_dir/out/current_link"
mkdir -p $build_output_dir
if [[ ! -d "$build_output_dir/BraveCore.xcframework" ]]; then
  cp -R "$src_dir/brave/ios/brave-ios/BraveCore/placeholders/." \
    "$build_output_dir/"
fi
touch "$build_output_dir/args.xcconfig"
