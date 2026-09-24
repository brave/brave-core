#!/bin/bash

# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This scripts gets executed by Omaha 4 when it installs a new version of Brave.
# It has special logic to address the following problem: We are in the process
# of migrating Brave's automatic update implementation from Sparkle to Omaha 4.
# The browser determines which one gets used via a Griffin flag. This works when
# there is a single user. But in system-wide installations with multiple users,
# the two auto-update implementations may try to update Brave at the same time.
# It is unlikely, but in the worst case could lead to a broken installation. The
# logic below aims to prevent this.

# Function to read Info.plist (copied from .install.sh).
# __CFPREFERENCES_AVOID_DAEMON prevents macOS from returning a stale value.
infoplist_read() {
  __CFPREFERENCES_AVOID_DAEMON=1 defaults read "${@}"
}

# Check if this is a system-wide installation:
if [[ ${EUID} -eq 0 ]]; then
  # When Sparkle starts downloading an update, it creates the file
  # /private/tmp/{cfbundleidentifier}.Sparkle.pid. We can use this to detect
  # whether Sparkle is active on the system. If yes, we abort this installation
  # and let Sparkle take precedence.
  if [[ ${#} -ge 2 ]]; then
    app_plist="${2}/Contents/Info"
    cfbundleidentifier="$(infoplist_read "${app_plist}" "CFBundleIdentifier" \
                        2>/dev/null)"
    if [[ -n "${cfbundleidentifier}" ]]; then
      sparkle_pid_file="/private/tmp/${cfbundleidentifier}.Sparkle.pid"
      # Sparkle deletes the file when it next gets loaded. macOS also clears
      # the contents of /private/tmp on reboot and periodically via
      # tmp_cleaner. It is therefore enough to check for the file's presence;
      # We don't need to consider the file's age or it staying around forever.
      if [[ -f "${sparkle_pid_file}" ]]; then
        # Sparkle is active on this system. Abort this installation with Omaha
        # 4 to prevent conflicts. Use a "rare" exit code to make it easier to
        # grep for.
        exit 249
      fi
    fi
  fi
fi
