/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/search/ntp_features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace ntp_features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kCustomizeChromeSidePanelExtensionsCard,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kCustomizeChromeWallpaperSearch, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNtpAlphaBackgroundCollections, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNtpBackgroundImageErrorDetection, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNtpChromeCartModule, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNtpModulesMaxColumnCount, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace ntp_features
