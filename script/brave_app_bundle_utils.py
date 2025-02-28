#!/usr/bin/env vpython3
#
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */


# Here we can extend the list of non-base services that are allowed to be
# included in the app bundle.
def extend_allowlisted_non_base_services(allowlisted_non_base_services):
    allowlisted_non_base_services.add(
        'com.brave.playlist.playback_service.VideoPlaybackService')
    return allowlisted_non_base_services
