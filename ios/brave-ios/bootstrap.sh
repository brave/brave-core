#!/bin/sh

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#
# Use the --ci option to use `npm ci` over `npm install`

set -e

missingCommand() {
    echo >&2 "Brave requires the command: \033[1m$1\033[0m\nPlease install it via Homebrew or directly from $2"
    exit 1
}

# First Check to see if they have the neccessary software installed
command -v swiftlint >/dev/null 2>&1 || { missingCommand "swiftlint" "https://github.com/realm/SwiftLint/releases"; }
command -v npm >/dev/null 2>&1 || { missingCommand "npm" "https://nodejs.org/en/download/"; }

# Log Colors
COLOR_ORANGE='\033[0;33m'
COLOR_NONE='\033[0m'

# Install Node.js dependencies and build user scripts

if [ "$1" == --ci ]; then
  npm ci
else
  npm install
fi

# Delete Chromium Assets from BraveCore.framework since they aren't used.
# TODO: Get this removed in the brave-core builds if possible
echo "${COLOR_ORANGE}Cleaning up BraveCore framework assets…${COLOR_NONE}"
find "node_modules/brave-core-ios" -name 'BraveCore.framework' -print0 | while read -d $'\0' framework
do
  if [[ -f "$framework/Assets.car" ]]; then
    rm "$framework/Assets.car"
  fi
done

# Codesign BraveCore + MaterialComponents to pass library validation on unit tests on M1 machines
echo "${COLOR_ORANGE}Signing BraveCore frameworks…${COLOR_NONE}"
find "node_modules/brave-core-ios" -name '*.framework' -print0 | while read -d $'\0' framework
do
  # MaterialComponents.framework doesn't seem to have a `CFBundleShortVersionString`
  /usr/libexec/PlistBuddy -c 'Add :CFBundleShortVersionString string 1.0' "${framework}/Info.plist" || true
  codesign --force --deep --sign "-" --preserve-metadata=identifier,entitlements --timestamp=none "${framework}"
done

npm run build

# Setup local git config
git config --local blame.ignoreRevsFile .git-blame-ignore-revs

# Sets up local configurations from the tracked .template files

# Checking the `Local` Directory
CONFIG_PATH="App/Configuration"
OLD_CONFIG_PATH="Client/Configuration"

if [ ! -d "$CONFIG_PATH/Local/" ]; then
  echo "${COLOR_ORANGE}Creating 'Local' directory${COLOR_NONE}"

  (cd $CONFIG_PATH && mkdir Local)
fi

if [ -d "$OLD_CONFIG_PATH/Local" ]; then
  echo "${COLOR_ORANGE}Copying configurations from old configuration directory${COLOR_NONE}"
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
