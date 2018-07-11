#!/bin/sh

#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#
# Bootstrap the Carthage dependencies. If the Carthage directory
# already exists then nothing is done. This speeds up builds on
# CI services where the Carthage directory can be cached.
#
# Use the --force option to force a rebuild of the dependencies.
#

if [ "$1" == "--force" ]; then
    rm -rf Carthage/*
    rm -rf ~/Library/Caches/org.carthage.CarthageKit
fi

# Only enable this on the Xcode Server because it times out if it does not
# get any output for some time while building the dependencies.

CARTHAGE_VERBOSE=""
if [ ! -z "$XCS_BOT_ID"  ]; then
  CARTHAGE_VERBOSE="--verbose"
fi

carthage bootstrap $CARTHAGE_VERBOSE --platform ios --color auto --cache-builds

# Install Node.js dependencies and build user scripts

npm install
npm run build

# Sets up local configurations from the tracked .template files

# Checking the `Local` Directory
CONFIG_PATH="Client/Configuration"
if [ ! -d "$CONFIG_PATH/Local/" ]; then
  echo "Creating 'Local' directory"

  (cd $CONFIG_PATH && mkdir Local)
fi

# Copying over any necessary files into `Local`
for CONFIG_FILE_NAME in BundleId DevTeam BuildId
do
  CONFIG_FILE=$CONFIG_FILE_NAME.xcconfig
  (cd $CONFIG_PATH \
    && cp -n Local.templates/$CONFIG_FILE Local/$CONFIG_FILE \
  )
done