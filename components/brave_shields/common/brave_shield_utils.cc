/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/common/brave_shield_utils.h"

#include "components/content_settings/core/common/content_settings_pattern.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

ContentSetting GetBraveFPContentSettingFromRules(
    const ContentSettingsForOneType& fp_rules,
    const GURL& primary_url) {
  absl::optional<ContentSettingPatternSource> global_fp_rule;
  absl::optional<ContentSettingPatternSource> global_fp_balanced_rule;

  for (const auto& rule : fp_rules) {
    if (rule.primary_pattern != ContentSettingsPattern::Wildcard() &&
        rule.primary_pattern.Matches(primary_url)) {
      if (rule.secondary_pattern ==
          ContentSettingsPattern::FromString("https://balanced")) {
        return CONTENT_SETTING_DEFAULT;
      }
      if (rule.secondary_pattern == ContentSettingsPattern::Wildcard())
        return rule.GetContentSetting();
    }

    if (rule.primary_pattern == ContentSettingsPattern::Wildcard()) {
      if (rule.secondary_pattern ==
          ContentSettingsPattern::FromString("https://balanced")) {
        DCHECK(!global_fp_rule);
        global_fp_balanced_rule = rule;
      }
      if (rule.secondary_pattern == ContentSettingsPattern::Wildcard()) {
        DCHECK(!global_fp_balanced_rule);
        global_fp_rule = rule;
      }
    }
  }

  if (global_fp_balanced_rule)
    return CONTENT_SETTING_DEFAULT;

  if (global_fp_rule)
    return global_fp_rule->GetContentSetting();

  return CONTENT_SETTING_DEFAULT;
}
