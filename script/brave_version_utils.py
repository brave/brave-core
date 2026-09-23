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


def generate_base_version_code(build_number, patch_number):
    # Version codes are laid out as PP_BB_PPP_CD where PP is a 2-digit
    # prefix, BB are the lower 2 digits of build_number, PPP is
    # patch_number, and CD is the package/ABI suffix. The prefix grows
    # past 42 once build_number exceeds 99 so that codes stay strictly
    # increasing without overflowing Android's int32 versionCode.
    prefix = 42 + build_number // 100
    return int('%02d%02d%03d00' % (prefix, build_number % 100, patch_number))
