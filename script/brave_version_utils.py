#!/usr/bin/env vpython3
#
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */


def remap_build_number(build_number):
    # Starting from Chromium's build number 5750 there is a change how
    # we generate version codes in order to provide proper app version
    # to the intel devices that support arm as well. Since we patch
    # build number with our version, we need to remap it.
    # We want to introduce this change starting from 1.53.x.
    if build_number >= 53:
        build_number = 5750
    return build_number
