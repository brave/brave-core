/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_default_provider.h"

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_partition_key.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "url/gurl.h"

namespace content_settings {

void BraveDefaultProvider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  DefaultProvider::RegisterProfilePrefs(registry);
}

BraveDefaultProvider::BraveDefaultProvider(PrefService* prefs,
                                           bool off_the_record,
                                           bool should_record_metrics)
    : DefaultProvider(prefs, off_the_record, should_record_metrics),
      prefs_(prefs) {
  pref_change_registrar_.Init(prefs);

  pref_change_registrar_.Add(
      brave_shields::prefs::kAdblockAdBlockOnlyModeState,
      base::BindRepeating(&BraveDefaultProvider::OnAdBlockOnlyModeChanged,
                          base::Unretained(this)));
}

BraveDefaultProvider::~BraveDefaultProvider() = default;

std::unique_ptr<Rule> BraveDefaultProvider::GetRule(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    bool off_the_record,
    const PartitionKey& partition_key) const {
  std::unique_ptr<Rule> rule = DefaultProvider::GetRule(
      primary_url, secondary_url, content_type, off_the_record, partition_key);
  return MaybeAdjustRuleForAdBlockOnlyMode(std::move(rule), primary_url,
                                           content_type);
}

void BraveDefaultProvider::OnAdBlockOnlyModeChanged() {
  base::AutoLock lock(lock_);
  ad_block_only_mode_enabled_ =
      brave_shields::GetBraveShieldsAdBlockOnlyModeEnabled(prefs_);
}

std::unique_ptr<Rule> BraveDefaultProvider::MaybeAdjustRuleForAdBlockOnlyMode(
    std::unique_ptr<Rule> rule,
    const GURL& primary_url,
    ContentSettingsType content_type) const {
  switch (content_type) {
    case ContentSettingsType::JAVASCRIPT:
      return MaybeSetRuleValueForAdBlockOnlyMode(std::move(rule), primary_url,
                                                 CONTENT_SETTING_ALLOW);
    case ContentSettingsType::COOKIES:
      return MaybeSetRuleValueForAdBlockOnlyMode(std::move(rule), primary_url,
                                                 CONTENT_SETTING_ALLOW);
    case ContentSettingsType::BRAVE_COOKIES:
      return MaybeSetRuleValueForAdBlockOnlyMode(std::move(rule), primary_url,
                                                 CONTENT_SETTING_ALLOW);
    default:
      break;
  }
  return rule;
}

std::unique_ptr<Rule> BraveDefaultProvider::MaybeSetRuleValueForAdBlockOnlyMode(
    std::unique_ptr<Rule> rule,
    const GURL& primary_url,
    ContentSetting content_setting) const {
  base::AutoLock lock(lock_);
  if (ad_block_only_mode_enabled_ && rule && primary_url.is_valid() &&
      primary_url.SchemeIsHTTPOrHTTPS()) {
    LOG(ERROR) << "FOOBAR.BraveDefaultProvider: " << primary_url.spec() << " "
               << content_setting;
    // rule->value = ContentSettingToValue(content_setting);
  }

  return rule;
}

}  // namespace content_settings
