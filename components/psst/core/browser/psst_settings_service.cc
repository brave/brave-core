/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/core/browser/psst_settings_service.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace psst {

namespace {

// How long the infobar stays suppressed for all origins after the user
// dismisses it without reviewing suggestions.
constexpr base::TimeDelta kInfobarDismissalSuppression = base::Days(30);

constexpr char kUserIdSettingsKey[] = "user_id";
constexpr char kConsentStatusSettingsKey[] = "consent_status";
constexpr char kScriptVersionSettingsKey[] = "script_version";
constexpr char kUidsToPerformSettingsKey[] = "uids_to_perform";
constexpr char kInfobarShownCounterSettingsKey[] = "infobar_shown_counter";
constexpr char kInfobarDeclineCounterSettingsKey[] = "infobar_decline_counter";
constexpr char kInfobarReviewSuggCounterSettingsKey[] =
    "infobar_review_sugg_counter";

base::ListValue VectorToList(std::vector<std::string> values) {
  base::ListValue list;
  list.reserve(values.size());
  for (auto& value : values) {
    list.Append(std::move(value));
  }
  return list;
}

base::DictValue CreatePsstSettingsObject(PsstWebsiteSettings psst_metadata) {
  base::DictValue object;
  object.Set(kUserIdSettingsKey, psst_metadata.user_id);
  object.Set(kConsentStatusSettingsKey, ToString(psst_metadata.consent_status));
  object.Set(kScriptVersionSettingsKey, psst_metadata.script_version);
  object.Set(kUidsToPerformSettingsKey,
             VectorToList(std::move(psst_metadata.uids_to_perform)));
  return object;
}

int GetInfobarCounterValue(const url::Origin& origin,
                           std::string_view setting_key,
                           HostContentSettingsMap& host_content_settings_map) {
  auto metadata_objects = host_content_settings_map.GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);
  const auto* metadata_objects_dict = metadata_objects.GetIfDict();
  if (!metadata_objects_dict) {
    return 0;
  }

  return metadata_objects_dict->FindInt(setting_key).value_or(0);
}

void IncrementInfobarCounter(
    const url::Origin& origin,
    std::string_view setting_key,
    HostContentSettingsMap& host_content_settings_map) {
  if (origin.scheme() != url::kHttpsScheme) {
    return;
  }

  auto metadata_objects = host_content_settings_map.GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);
  auto* metadata_objects_dict = metadata_objects.GetIfDict();
  base::DictValue updated_dict = metadata_objects_dict
                                     ? std::move(*metadata_objects_dict)
                                     : base::DictValue();
  const int count = updated_dict.FindInt(setting_key).value_or(0);
  updated_dict.Set(setting_key, count + 1);
  host_content_settings_map.SetWebsiteSettingDefaultScope(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST,
      base::Value(std::move(updated_dict)));
}

}  // namespace

PsstSettingsService::PsstSettingsService(
    HostContentSettingsMap& host_content_settings_map,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map), prefs_(prefs) {
  CHECK(prefs_);

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kPsstEnabled,
      base::BindRepeating(&PsstSettingsService::OnPreferenceChanged,
                          weak_ptr_factory_.GetWeakPtr()));
}

PsstSettingsService::~PsstSettingsService() = default;

void PsstSettingsService::AddObserver(PrefObserver* observer) {
  observers_.AddObserver(observer);
}

void PsstSettingsService::RemoveObserver(PrefObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::optional<PsstWebsiteSettings> PsstSettingsService::GetPsstWebsiteSettings(
    const url::Origin& origin,
    std::string_view user_id) {
  auto metadata_objects = host_content_settings_map_->GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);
  auto* metadata_objects_dict = metadata_objects.GetIfDict();
  if (!metadata_objects_dict) {
    return std::nullopt;
  }

  auto* user_id_metadata_dict = metadata_objects_dict->FindDict(user_id);
  if (!user_id_metadata_dict) {
    return std::nullopt;
  }

  return PsstWebsiteSettings::FromValue(*user_id_metadata_dict);
}

base::flat_map<std::string, PsstWebsiteSettingsExtended>
PsstSettingsService::GetAllPsstWebsitesSettings() {
  ContentSettingsForOneType settings =
      host_content_settings_map_->GetSettingsForOneType(
          ContentSettingsType::BRAVE_PSST);

  base::flat_map<std::string, PsstWebsiteSettingsExtended> result;
  for (const auto& setting : settings) {
    const auto url = GURL(setting.primary_pattern.ToString());
    if (!url.is_valid()) {
      continue;
    }

    const auto* website_settings_dict = setting.setting_value.GetIfDict();
    if (!website_settings_dict) {
      continue;
    }

    PsstWebsiteSettingsExtended extended;
    extended.infobar_shown_counter =
        website_settings_dict->FindInt(kInfobarShownCounterSettingsKey)
            .value_or(0);
    extended.infobar_decline_counter =
        website_settings_dict->FindInt(kInfobarDeclineCounterSettingsKey)
            .value_or(0);
    extended.infobar_review_sugg_counter =
        website_settings_dict->FindInt(kInfobarReviewSuggCounterSettingsKey)
            .value_or(0);

    for (const auto [key, value] : *website_settings_dict) {
      if (key == kInfobarShownCounterSettingsKey ||
          key == kInfobarDeclineCounterSettingsKey) {
        continue;
      }

      const auto* user_id_metadata_dict = value.GetIfDict();
      if (!user_id_metadata_dict) {
        continue;
      }

      if (auto psst_metadata =
              PsstWebsiteSettings::FromValue(*user_id_metadata_dict)) {
        extended.website_settings.push_back(std::move(*psst_metadata));
      }
    }

    result.emplace(url.host(), std::move(extended));
  }
  return result;
}

void PsstSettingsService::SetPsstWebsiteSettings(
    const url::Origin& origin,
    ConsentStatus consent_status,
    int script_version,
    std::string_view user_id,
    base::ListValue uids_to_perform) {
  auto psst_metadata = PsstWebsiteSettings::FromValue(
      base::DictValue()
          .Set(kUserIdSettingsKey, user_id)
          .Set(kConsentStatusSettingsKey, ToString(consent_status))
          .Set(kScriptVersionSettingsKey, script_version)
          .Set(kUidsToPerformSettingsKey, std::move(uids_to_perform)));
  if (!psst_metadata) {
    return;
  }

  SetPsstWebsiteSettings(origin, std::move(*psst_metadata));
}

void PsstSettingsService::SetPsstWebsiteSettings(
    const url::Origin& origin,
    PsstWebsiteSettings psst_metadata) {
  if (origin.scheme() != url::kHttpsScheme) {
    return;
  }

  auto metadata_objects = host_content_settings_map_->GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);

  const auto user_id = psst_metadata.user_id;
  auto* metadata_objects_dict = metadata_objects.GetIfDict();
  if (metadata_objects_dict) {
    metadata_objects_dict->Set(
        user_id, CreatePsstSettingsObject(std::move(psst_metadata)));
    host_content_settings_map_->SetWebsiteSettingDefaultScope(
        origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST,
        base::Value(std::move(*metadata_objects_dict)));
  } else {
    host_content_settings_map_->SetWebsiteSettingDefaultScope(
        origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST,
        base::Value(base::DictValue().Set(
            user_id, CreatePsstSettingsObject(std::move(psst_metadata)))));
  }
  observers_.Notify(&PsstSettingsService::PrefObserver::OnPsstPrefsChange,
                    GetAllPsstWebsitesSettings());
}

int PsstSettingsService::GetInfobarShownCount(const url::Origin& origin) {
  return GetInfobarCounterValue(origin, kInfobarShownCounterSettingsKey,
                                *host_content_settings_map_);
}

int PsstSettingsService::GetInfobarDeclinedCount(const url::Origin& origin) {
  return GetInfobarCounterValue(origin, kInfobarDeclineCounterSettingsKey,
                                *host_content_settings_map_);
}

int PsstSettingsService::GetInfobarReviewSuggestionsCount(const url::Origin& origin) {
  return GetInfobarCounterValue(origin, kInfobarReviewSuggCounterSettingsKey,
                                *host_content_settings_map_);
}

void PsstSettingsService::IncrementInfobarShownCount(
    const url::Origin& origin) {
  IncrementInfobarCounter(origin, kInfobarShownCounterSettingsKey,
                          *host_content_settings_map_);
  observers_.Notify(&PsstSettingsService::PrefObserver::OnPsstPrefsChange,
                    GetAllPsstWebsitesSettings());
}

void PsstSettingsService::IncrementInfobarDeclineCount(
    const url::Origin& origin) {
  IncrementInfobarCounter(origin, kInfobarDeclineCounterSettingsKey,
                          *host_content_settings_map_);
  observers_.Notify(&PsstSettingsService::PrefObserver::OnPsstPrefsChange,
                    GetAllPsstWebsitesSettings());
}

void PsstSettingsService::RecordInfobarDismissed() {
  prefs_->SetTime(prefs::kPsstInfobarLastDismissedTime, base::Time::Now());
}

bool PsstSettingsService::IsInfobarDismissalSuppressed() const {
  const base::Time last_dismissed_time =
      prefs_->GetTime(prefs::kPsstInfobarLastDismissedTime);
  return !last_dismissed_time.is_null() &&
         base::Time::Now() - last_dismissed_time < kInfobarDismissalSuppression;
}

void PsstSettingsService::IncrementInfobarReviewSuggestionsCount(
    const url::Origin& origin) {
  IncrementInfobarCounter(origin, kInfobarReviewSuggCounterSettingsKey,
                          *host_content_settings_map_);
  observers_.Notify(&PsstSettingsService::PrefObserver::OnPsstPrefsChange,
                    GetAllPsstWebsitesSettings());
}

bool PsstSettingsService::IsPsstEnabled() const {
  return prefs_->GetBoolean(prefs::kPsstEnabled);
}

void PsstSettingsService::SetPsstEnabled(bool enabled) {
  prefs_->SetBoolean(prefs::kPsstEnabled, enabled);
}

void PsstSettingsService::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == prefs::kPsstEnabled) {
    observers_.Notify(&PrefObserver::OnPsstEnableChange, IsPsstEnabled());
    return;
  }
}

}  // namespace psst
