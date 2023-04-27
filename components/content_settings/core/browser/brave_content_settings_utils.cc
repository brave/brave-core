/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"

namespace content_settings {
namespace {

constexpr auto kBraveContentSettingsTypes =
    base::MakeFixedFlatSet<ContentSettingsType>({
        ContentSettingsType::BRAVE_ADS,
        ContentSettingsType::BRAVE_COSMETIC_FILTERING,
        ContentSettingsType::BRAVE_TRACKERS,
        ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
        ContentSettingsType::BRAVE_FINGERPRINTING_V2,
        ContentSettingsType::BRAVE_SHIELDS,
        ContentSettingsType::BRAVE_REFERRERS,
        ContentSettingsType::BRAVE_COOKIES,
        ContentSettingsType::BRAVE_SPEEDREADER,
        ContentSettingsType::BRAVE_ETHEREUM,
        ContentSettingsType::BRAVE_SOLANA,
        ContentSettingsType::BRAVE_GOOGLE_SIGN_IN,
        ContentSettingsType::BRAVE_HTTPS_UPGRADE,
        ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
        ContentSettingsType::BRAVE_LOCALHOST_ACCESS,
    });

}  // namespace

base::span<const ContentSettingsType> GetBraveContentSettingsTypes() {
  return base::make_span(kBraveContentSettingsTypes.begin(),
                         kBraveContentSettingsTypes.end());
}

bool IsBraveContentSettingsType(const ContentSettingsType& content_type) {
  return base::Contains(kBraveContentSettingsTypes, content_type);
}

}  // namespace content_settings
