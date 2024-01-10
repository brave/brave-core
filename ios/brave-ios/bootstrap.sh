#!/bin/sh

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# TODO(@brave/ios): Move contents of this into `npm run sync` command

set -e

# Log Colors
COLOR_ORANGE='\033[0;33m'
COLOR_NONE='\033[0m'

# Install Node.js dependencies and build user scripts

npm run ios_pack_js

# Set up BraveCore placeholders to allow SPM to validate the package
# This folder will be replaced on first build to a symlink to the current
# build config
build_output_dir="../../../out/current_link"
mkdir -p $build_output_dir
if [[ ! -d "$build_output_dir/BraveCore.xcframework" ]]; then
  cp -R "BraveCore/placeholders/." "$build_output_dir/"
fi
touch "$build_output_dir/args.xcconfig"

# Sets up local configurations from the tracked .template files

# Checking the `Local` Directory
CONFIG_PATH="App/Configuration"
OLD_CONFIG_PATH="Client/Configuration"

if [ ! -d "$CONFIG_PATH/Local/" ]; then
  echo "${COLOR_ORANGE}Creating 'Local' directory${COLOR_NONE}"

  (cd $CONFIG_PATH && mkdir Local)
fi

if [ -d "$OLD_CONFIG_PATH/Local" ]; then
  echo "${COLOR_ORANGE}Copying configurations from old configuration \
    directory${COLOR_NONE}"
  for CONFIG_FILE in $OLD_CONFIG_PATH/Local/*.xcconfig
  do
    if cp -n $CONFIG_FILE $CONFIG_PATH/Local/ ; then
      rm $CONFIG_FILE
    fi
  done
  rm -rf "$OLD_CONFIG_PATH"
fi

# Copying over any necessary files into `Local`
for CONFIG_FILE_TEMPLATE in $CONFIG_PATH/Local.templates/*.xcconfig
do
  echo "${COLOR_ORANGE}Attempting to copy $CONFIG_FILE_TEMPLATE${COLOR_NONE}"
  # `|| true` is used to force continuation if cp fails for a specific item (e.g. already exists)
  cp -n $CONFIG_FILE_TEMPLATE $CONFIG_PATH/Local/ || true
done
