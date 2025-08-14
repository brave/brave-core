// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/common/prefs.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/psst/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace psst::prefs {

namespace {

constexpr char kConsentStatus[] = "consent_status";
constexpr char kScriptVersion[] = "script_version";
constexpr char kUrlsToSkip[] = "urls_to_skip";

const base::Value* GetPsstSettingsDict(const std::string& name,
                                       const std::string& user_id,
                                       const std::string& prop,
                                       const PrefService& prefs) {
  const auto& psst_settings = prefs.GetDict(prefs::kPsstSettingsPref);
  return psst_settings.FindByDottedPath(
      base::JoinString({name, user_id, prop}, "."));
}

}  // namespace

std::optional<ConsentStatus> GetConsentStatus(const std::string& name,
                                              const std::string& user_id,
                                              const PrefService& prefs) {
  const base::Value* consent_status_val =
      GetPsstSettingsDict(name, user_id, kConsentStatus, prefs);
  if (!consent_status_val || !consent_status_val->is_int()) {
    return std::nullopt;
  }

  return static_cast<ConsentStatus>(consent_status_val->GetInt());
}

std::optional<int> GetScriptVersion(const std::string& name,
                                    const std::string& user_id,
                                    const PrefService& prefs) {
  const base::Value* consent_status_val =
      GetPsstSettingsDict(name, user_id, kScriptVersion, prefs);
  if (!consent_status_val || !consent_status_val->is_int()) {
    return std::nullopt;
  }
  return consent_status_val->GetInt();
}

std::optional<base::Value::List> GetUrlsToSkip(const std::string& name,
                                               const std::string& user_id,
                                               const PrefService& prefs) {
  const base::Value* urls_to_skip_val =
      GetPsstSettingsDict(name, user_id, kUrlsToSkip, prefs);
  if (!urls_to_skip_val || !urls_to_skip_val->is_list()) {
    return std::nullopt;
  }

  return urls_to_skip_val->GetList().Clone();
}

void SetPsstSettings(const std::string& name,
                     const std::string& user_id,
                     std::optional<ConsentStatus> consent_status,
                     std::optional<int> script_version,
                     std::optional<base::Value::List> urls_to_skip,
                     PrefService& prefs) {
  ScopedDictPrefUpdate update(&prefs, prefs::kPsstSettingsPref);
  const std::string path = base::JoinString({name, user_id}, ".");

  if (consent_status) {
    update->SetByDottedPath(base::JoinString({path, kConsentStatus}, "."),
                            static_cast<int>(*consent_status));
  }

  if (script_version) {
    update->SetByDottedPath(base::JoinString({path, kScriptVersion}, "."),
                            *script_version);
  }

  if (urls_to_skip) {
    update->SetByDottedPath(base::JoinString({path, kUrlsToSkip}, "."),
                            urls_to_skip->Clone());
  }
}

}  // namespace psst::prefs
