/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_policy_provider.h"

#include "base/containers/contains.h"
#include "base/logging.h"
#include "brave/components/constants/pref_names.h"
#include "components/content_settings/core/browser/content_settings_policy_provider.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace {

// Duplicating declaration from
// components/content_settings/core/browser/content_settings_policy_provider.cc
struct PrefsForManagedContentSettingsMapEntry {
  const char* pref_name;
  ContentSettingsType content_type;
  ContentSetting setting;
};

constexpr PrefsForManagedContentSettingsMapEntry
    kBravePrefsForManagedContentSettingsMap[] = {
        {kManagedBraveShieldsDisabledForUrls,
         ContentSettingsType::BRAVE_SHIELDS, CONTENT_SETTING_BLOCK},
        {kManagedBraveShieldsEnabledForUrls, ContentSettingsType::BRAVE_SHIELDS,
         CONTENT_SETTING_ALLOW}};

constexpr const char* kBraveManagedPrefs[] = {
    kManagedBraveShieldsDisabledForUrls, kManagedBraveShieldsEnabledForUrls};

}  // namespace

namespace content_settings {

BravePolicyProvider::BravePolicyProvider(PrefService* prefs)
    : PolicyProvider(prefs) {
  ReadManagedDefaultSettings();
  ReadManagedContentSettings(false);
  pref_change_registrar_.Init(prefs_);
  PrefChangeRegistrar::NamedChangeCallback callback = base::BindRepeating(
      &BravePolicyProvider::OnPreferenceChanged, base::Unretained(this));
  for (const char* pref : kBraveManagedPrefs)
    pref_change_registrar_.Add(pref, callback);
}

BravePolicyProvider::~BravePolicyProvider() = default;

// static
void BravePolicyProvider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  PolicyProvider::RegisterProfilePrefs(registry);
  for (const char* pref : kBraveManagedPrefs)
    registry->RegisterListPref(pref);
}

void BravePolicyProvider::OnPreferenceChanged(const std::string& name) {
  if (base::Contains(kBraveManagedPrefs, name)) {
    ReadManagedContentSettings(true);
    ReadManagedDefaultSettings();
  }
  PolicyProvider::OnPreferenceChanged(name);
}

void BravePolicyProvider::ReadManagedContentSettings(bool overwrite) {
  PolicyProvider::ReadManagedContentSettings(overwrite);
  GetBraveContentSettingsFromPreferences(&value_map_);
}

/// Duplicating PolicyProvider::GetContentSettingsFromPreferences for
/// kBravePrefsForManagedContentSettingsMap
void BravePolicyProvider::GetBraveContentSettingsFromPreferences(
    OriginIdentifierValueMap* value_map) {
  for (const auto& entry : kBravePrefsForManagedContentSettingsMap) {
    // Skip unset policies.
    if (!prefs_->HasPrefPath(entry.pref_name)) {
      VLOG(2) << "Skipping unset preference: " << entry.pref_name;
      continue;
    }

    const PrefService::Preference* pref =
        prefs_->FindPreference(entry.pref_name);
    DCHECK(pref);
    DCHECK(!pref->HasUserSetting());
    DCHECK(!pref->HasExtensionSetting());

    if (!pref->GetValue()->is_list()) {
      NOTREACHED() << "Could not read patterns from " << entry.pref_name;
      return;
    }

    const base::Value::List& pattern_str_list = pref->GetValue()->GetList();
    for (size_t i = 0; i < pattern_str_list.size(); ++i) {
      if (!pattern_str_list[i].is_string()) {
        NOTREACHED() << "Could not read content settings pattern #" << i
                     << " from " << entry.pref_name;
        continue;
      }

      const std::string& original_pattern_str = pattern_str_list[i].GetString();
      VLOG(2) << "Reading content settings pattern " << original_pattern_str
              << " from " << entry.pref_name;

      PatternPair pattern_pair = ParsePatternString(original_pattern_str);
      // Ignore invalid patterns.
      if (!pattern_pair.first.IsValid()) {
        VLOG(1) << "Ignoring invalid content settings pattern "
                << original_pattern_str;
        continue;
      }

      DCHECK_NE(entry.content_type,
                ContentSettingsType::AUTO_SELECT_CERTIFICATE);
      // If only one pattern was defined auto expand it to a pattern pair.
      ContentSettingsPattern secondary_pattern =
          !pattern_pair.second.IsValid() ? ContentSettingsPattern::Wildcard()
                                         : pattern_pair.second;
      VLOG_IF(2, !pattern_pair.second.IsValid())
          << "Replacing invalid secondary pattern '"
          << pattern_pair.second.ToString() << "' with wildcard";

      // All settings that can set pattern pairs support embedded exceptions.
      if (pattern_pair.first != pattern_pair.second &&
          pattern_pair.second != ContentSettingsPattern::Wildcard() &&
          !WebsiteSettingsRegistry::GetInstance()
               ->Get(entry.content_type)
               ->SupportsSecondaryPattern()) {
        continue;
      }

      // Don't set a timestamp for policy settings.
      value_map->SetValue(pattern_pair.first, secondary_pattern,
                          entry.content_type, base::Value(entry.setting), {});
    }
  }
}

}  // namespace content_settings
