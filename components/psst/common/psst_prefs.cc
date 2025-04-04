// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/common/psst_prefs.h"

#include <optional>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/psst/common/features.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"

namespace psst {

namespace {

inline constexpr char kConsentStatus[] = "consent_status";
inline constexpr char kScriptVersion[] = "script_version";
inline constexpr char kUrlsToSkip[] = "urls_to_skip";
inline constexpr char kEnablePsstFlag[] = "enable_psst";

}  // namespace

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  if (base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    registry->RegisterBooleanPref(prefs::kPsstEnabled, true);
    registry->RegisterDictionaryPref(
        prefs::kPsstSettingsPref,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  }
}

// Construct a lookup key from the user id and name of the Matched Rule.
// The key is constructed as follows:
// <name>.<user_id>
// For example, if the user id is "user1" and the name is "twitter",
// the key will be "twitter.user1".
// See: base::Value::Dict::FindByDottedPath()
const std::string ConstructPath(const std::string& user_id,
                                const std::string& name) {
  return base::StringPrintf("%s.%s", name.c_str(), user_id.c_str());
}

PsstSettings::PsstSettings(const PsstConsentStatus consent_status,
                           const int script_version,
                           const std::vector<std::string>& urls_to_skip)
    : urls_to_skip(urls_to_skip),
      consent_status(consent_status),
      script_version(script_version) {}

PsstSettings::PsstSettings(const PsstConsentStatus consent_status,
                           const int script_version)
    : consent_status(consent_status), script_version(script_version) {}

PsstSettings::PsstSettings(const PsstSettings& other) = default;
PsstSettings::PsstSettings(PsstSettings&& other) = default;
PsstSettings::~PsstSettings() = default;

bool GetEnablePsstFlag(PrefService* prefs) {
  if (!prefs->HasPrefPath(prefs::kPsstSettingsPref)) {
    return false;
  }

  const auto& psst_settings = prefs->GetDict(prefs::kPsstSettingsPref);
  const auto result = psst_settings.FindBool(kEnablePsstFlag);
  return result.has_value() && result.value();
}

void SetEnablePsstFlag(PrefService* prefs, const bool val) {
  ScopedDictPrefUpdate update(prefs, prefs::kPsstSettingsPref);
  base::Value::Dict& pref = update.Get();
  pref.Set(kEnablePsstFlag, val);
  //  update->Set(kEnablePsstFlag, val);
}

std::optional<PsstSettings> GetPsstSettings(const std::string& user_id,
                                            const std::string& name,
                                            PrefService* prefs) {
  if (!prefs->HasPrefPath(prefs::kPsstSettingsPref)) {
    return std::nullopt;
  }

  const auto& psst_settings = prefs->GetDict(prefs::kPsstSettingsPref);
  const std::string path = ConstructPath(user_id, name);
  const base::Value* settings_for_site_val =
      psst_settings.FindByDottedPath(path);

  if (!settings_for_site_val || !settings_for_site_val->is_dict()) {
    return std::nullopt;
  }
  const base::Value::Dict& settings_for_site = settings_for_site_val->GetDict();
  auto status_int = settings_for_site.FindInt(kConsentStatus);
  DCHECK(status_int.has_value());
  PsstConsentStatus status = static_cast<PsstConsentStatus>(status_int.value());
  auto script_version = settings_for_site.FindInt(kScriptVersion);

  std::vector<std::string> urls;
  auto* urls_to_skip = settings_for_site.FindList(kUrlsToSkip);
  if (urls_to_skip) {
    for (auto& url : *urls_to_skip) {
      if (url.is_string()) {
        urls.push_back(url.GetString());
      }
    }
  }
  return PsstSettings(status, *script_version, urls);
}

base::Value::Dict PsstSettingsToDict(const PsstSettings settings) {
  base::Value::List urls_to_skip;
  for (const auto& url : settings.urls_to_skip) {
    urls_to_skip.Append(url);
  }
  return base::Value::Dict()
      .Set(kConsentStatus, settings.consent_status)
      .Set(kScriptVersion, settings.script_version)
      .Set(kUrlsToSkip, std::move(urls_to_skip));
}

base::Value* SetPsstSettings(const std::string& user_id,
                             const std::string& name,
                             const PsstSettings settings,
                             PrefService* prefs) {
  ScopedDictPrefUpdate update(prefs, prefs::kPsstSettingsPref);
  const std::string path = ConstructPath(user_id, name);
  auto value = PsstSettingsToDict(settings);
  return update->SetByDottedPath(path, std::move(value));
}

}  // namespace psst
