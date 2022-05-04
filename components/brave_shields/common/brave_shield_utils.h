/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_UTILS_H_

#include "components/content_settings/core/common/content_settings.h"

class GURL;

namespace brave_shields {

struct ShieldsSettingCounts {
  int allow;
  int standard;
  int aggressive;
};

ContentSetting GetBraveFPContentSettingFromRules(
    const ContentSettingsForOneType& fp_rules,
    const GURL& primary_url);

ShieldsSettingCounts GetFPSettingCountFromRules(
    const ContentSettingsForOneType& fp_rules);
ShieldsSettingCounts GetAdsSettingCountFromRules(
    const ContentSettingsForOneType& ads_rules);
}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_COMMON_BRAVE_SHIELD_UTILS_H_
