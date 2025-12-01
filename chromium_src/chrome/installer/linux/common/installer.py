# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


# This override:
# - Maps dev/unstable/alpha -> dev
# - Adds nightly as a supported channel
# - Returns the appropriate releaste notes link for all channels
@override_utils.override_function(globals())
def normalize_channel(original_function, channel):
    if channel == "stable":
        return "stable", "https://brave.com/latest/"
    if channel in ["beta", "testing"]:
        return "beta", "https://brave.com/latest/"
    if channel in ["dev", "unstable", "alpha"]:
        return "dev", "https://brave.com/latest/"
    if channel == "nightly":
        return "nightly", "https://brave.com/latest/"
    return original_function(channel)


# This override adds or modifies the following context values:
# - PACKAGEANDCHANNEL: required by our templates (drops channel name for stable)
# - USR_BIN_SYMLINK_NAME: required for docs (drops channel name for stable)
# - VERSION/VERSIONFULL: drops MAJOR from the version string
@override_utils.override_method(Installer)
def initialize(self, original_method):
    original_method(self)
    self.context["PACKAGEANDCHANNEL"] = self.context["PACKAGE"]
    self.context["USR_BIN_SYMLINK_NAME"] = self.context["PACKAGE"]
    self.context["VERSION"] = (f"{self.context['MINOR']}."
                               f"{self.context['BUILD']}."
                               f"{self.context['PATCH']}")
    self.context["VERSIONFULL"] = (f"{self.context['MINOR']}."
                                   f"{self.context['BUILD']}."
                                   f"{self.context['PATCH']}")
