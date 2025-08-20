/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/ad_block_only_mode_content_settings_utils.h"

#include "base/containers/fixed_flat_set.h"
#include "base/time/time.h"
#include "components/content_settings/core/browser/content_settings_origin_value_map.h"
#include "components/content_settings/core/common/content_settings_enums.mojom.h"
#include "components/content_settings/core/common/content_settings_metadata.h"
#include "components/content_settings/core/common/content_settings_utils.h"

namespace content_settings {

bool IsAdBlockOnlyModeContentSettingsType(ContentSettingsType content_type,
                                          bool is_off_the_record) {
  static constexpr auto kAdBlockOnlyModeContentSettingsTypes =
      base::MakeFixedFlatSet<ContentSettingsType>({
          ContentSettingsType::JAVASCRIPT,
          ContentSettingsType::COOKIES,
          ContentSettingsType::BRAVE_COOKIES,
          ContentSettingsType::BRAVE_REFERRERS,
          ContentSettingsType::BRAVE_ADS,
          ContentSettingsType::BRAVE_TRACKERS,
          ContentSettingsType::BRAVE_COSMETIC_FILTERING,
          ContentSettingsType::BRAVE_FINGERPRINTING_V2,
          ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
          ContentSettingsType::BRAVE_HTTPS_UPGRADE,
      });

  // These types are off the record aware so we don't override them in off the
  // record mode.
  static constexpr auto kOffTheRecordAwareTypes =
      base::MakeFixedFlatSet<ContentSettingsType>({
          ContentSettingsType::BRAVE_HTTPS_UPGRADE,
      });

  if (!kAdBlockOnlyModeContentSettingsTypes.contains(content_type)) {
    return false;
  }

  if (is_off_the_record && kOffTheRecordAwareTypes.contains(content_type)) {
    return false;
  }

  return true;
}

void FillAdBlockOnlyModeRules(OriginValueMap& ad_block_only_mode_rules) {
  RuleMetaData metadata;
  metadata.SetExpirationAndLifetime(base::Time(), base::TimeDelta());
  metadata.set_session_model(mojom::SessionModel::DURABLE);

  base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());

  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::COOKIES,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());
  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COOKIES,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());
  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_REFERRERS,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());

  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS,
      ContentSettingToValue(CONTENT_SETTING_BLOCK), metadata.Clone());
  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_TRACKERS,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());
  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());

  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());

  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      ContentSettingToValue(CONTENT_SETTING_ALLOW), metadata.Clone());

  ad_block_only_mode_rules.SetValue(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTPS_UPGRADE,
      ContentSettingToValue(CONTENT_SETTING_ASK), metadata.Clone());
}

}  // namespace content_settings
