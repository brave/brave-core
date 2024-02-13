/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/common/brave_shield_utils.h"

#include <set>
#include <string>

#include "base/no_destructor.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "url/gurl.h"

namespace brave_shields {

ContentSetting GetBraveFPContentSettingFromRules(
    const ContentSettingsForOneType& fp_rules,
    const GURL& primary_url) {
  static const base::NoDestructor<ContentSettingsPattern> kBalancedRule(
      ContentSettingsPattern::FromString("https://balanced"));
  ContentSettingPatternSource fp_rule;
  for (const auto& rule : fp_rules) {
    if (rule.secondary_pattern == *kBalancedRule)
      continue;
    if (rule.primary_pattern.Matches(primary_url)) {
      return rule.GetContentSetting();
    }
  }

  return CONTENT_SETTING_DEFAULT;
}

ShieldsSettingCounts GetFPSettingCountFromRules(
    const ContentSettingsForOneType& fp_rules) {
  ShieldsSettingCounts result = {};

  for (const auto& rule : fp_rules) {
    if (rule.primary_pattern.MatchesAllHosts()) {
      continue;
    }
    if (rule.GetContentSetting() == CONTENT_SETTING_ALLOW) {
      result.allow++;
    } else if (rule.GetContentSetting() == CONTENT_SETTING_BLOCK) {
      result.aggressive++;
    } else {
      result.standard++;
    }
  }

  return result;
}

ShieldsSettingCounts GetAdsSettingCountFromRules(
    const ContentSettingsForOneType& ads_rules) {
  ShieldsSettingCounts result = {};

  std::set<std::string> block_set;
  // Look at primary rules
  for (const auto& rule : ads_rules) {
    if (rule.primary_pattern.MatchesAllHosts() ||
        !rule.secondary_pattern.MatchesAllHosts()) {
      continue;
    }
    if (rule.GetContentSetting() == CONTENT_SETTING_ALLOW) {
      result.allow++;
    } else {
      block_set.insert(rule.primary_pattern.ToString());
    }
  }

  // And then look at "first party" rules
  for (const auto& rule : ads_rules) {
    if (rule.primary_pattern.MatchesAllHosts() ||
        rule.secondary_pattern.MatchesAllHosts() ||
        block_set.find(rule.primary_pattern.ToString()) == block_set.end()) {
      continue;
    }
    if (rule.GetContentSetting() == CONTENT_SETTING_BLOCK) {
      result.aggressive++;
    } else {
      result.standard++;
    }
  }

  return result;
}

}  // namespace brave_shields
