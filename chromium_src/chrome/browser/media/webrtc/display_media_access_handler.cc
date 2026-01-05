/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <chrome/browser/media/webrtc/display_media_access_handler.cc>

// Upstream enabled this feature via finch field trial. We need this feature
// enabled as well as it addresses a security exploit.
OVERRIDE_FEATURE_DEFAULT_STATES({{
#if !BUILDFLAG(IS_ANDROID)
    // Upstream field trial only enabled this feature on desktop platforms.
    {kDisplayMediaRejectLongDomains, base::FEATURE_ENABLED_BY_DEFAULT},
#endif
}});
