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
  npm install
else
  npm ci
fi

npm run build

# Sets up local configurations from the tracked .template files

# Checking the `Local` Directory
CONFIG_PATH="Client/Configuration"
if [ ! -d "$CONFIG_PATH/Local/" ]; then
  echo "${COLOR_ORANGE}Creating 'Local' directory${COLOR_NONE}"

  (cd $CONFIG_PATH && mkdir Local)
fi

# Copying over any necessary files into `Local`
for CONFIG_FILE_TEMPLATE in $CONFIG_PATH/Local.templates/*.xcconfig
do
  echo "${COLOR_ORANGE}Attempting to copy $CONFIG_FILE_TEMPLATE${COLOR_NONE}"
  # `|| true` is used to force continuation if cp fails for a specific item (e.g. already exists)
  cp -n $CONFIG_FILE_TEMPLATE $CONFIG_PATH/Local/ || true
done
