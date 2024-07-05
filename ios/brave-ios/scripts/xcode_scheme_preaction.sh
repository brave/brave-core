# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script is run by Xcode and assumes Xcode build settings are injected
# into the environment.

# $ACTION is empty for Xcode builds unfortunately
# $RUN_CLANG_STATIC_ANALYZER is NO for builds, YES for cleans
if [[ $RUN_CLANG_STATIC_ANALYZER = "NO" ]]; then
  if ! command -v npm &> /dev/null; then
    # Fixup PATH for users who didn't install node directly or use nvm
    if [[ -s "$HOME/.nvm/nvm.sh" ]]; then
      . "$HOME/.nvm/nvm.sh"
    else
        BREW='brew'
        if [[ $(uname -p) == 'arm' ]]; then
            # On Apple Silicon (M1, M2, M3) Brew is not part
            # of the PATH.
            BREW='/opt/homebrew/bin/brew'
        fi
      if [[ -x "$(command -v $BREW)" ]]; then
        if [[ -s "$($BREW --prefix nvm)/nvm.sh" ]]; then
          . "$($BREW --prefix nvm)/nvm.sh"
        else
          # Fixup PATH for brew users
          eval "$($BREW shellenv)"
        fi
      fi
    fi
  fi
  # Do not inject Xcode build configs into the GN build
  env -i PATH="$PATH" python3 \
    "${PROJECT_DIR}/../scripts/scheme_preaction.py" \
    --configuration "$CONFIGURATION" \
    --platform_name "$PLATFORM_NAME" \
    "$@"
fi
