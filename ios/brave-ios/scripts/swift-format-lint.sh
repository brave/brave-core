# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script is run by Xcode and assumes Xcode build settings are injected
# into the environment.

depot_tools_path=$(realpath "../../../vendor/depot_tools")
PATH="$PATH:$depot_tools_path"
brave_ios_dir="ios/brave-ios/"

if ! [[ -f "$depot_tools_path/swift-format" ]]; then
  echo "Could not find swift-format binary"
  exit
fi

# Builds a git diff command similar to git-cl format
git -c core.quotePath=false diff \
  --no-ext-diff \
  --no-prefix \
  --name-only \
  --diff-filter=crd \
  origin/master | while read filename; do
  # Since this is run in Xcode, only lint Swift files that exist inside of the
  # Xcode project.
  if [[ "$filename" = $brave_ios_dir* && "${filename#*.}" = "swift" ]]; then
    # We have to make this path relative so that Xcode knows how to map
    # the real source
    swift-format lint "../${filename#"$brave_ios_dir"}"
  fi
done
