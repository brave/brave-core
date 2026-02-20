/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <media/base/media_switches.cc>

namespace media {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    // kLiveCaption is no longer disabled: the SODA (Speech On-Device API)
    // infrastructure must be available for on-device speech recognition
    // via the WebSpeech API. Live Caption UI is removed in Brave's settings
    // (a11y_page.ts), so the feature won't be visible to users.
    {kEnableTabMuting, base::FEATURE_ENABLED_BY_DEFAULT},
}});

}  // namespace media
