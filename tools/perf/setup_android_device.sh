#!/bin/sh
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# The script is used to setup the android device:
# 1. Disable SELinux
# 2. Enable adb root (adbd must be restarted after)
# 3. Unlock the device screen

setenforce 0 && \
resetprop ro.debuggable 1 && \
magiskpolicy --live "allow adbd adbd process setcurrent" && \
magiskpolicy --live "allow adbd su process dyntransition" && \
magiskpolicy --live "permissive { su }" && \
input keyevent 82 && \
rm -f /data/local/tmp/chrome-command-line
