/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/psst_settings_service.h"

#include "url/gurl.h"

namespace psst {

namespace {

constexpr char kUserIdSettingsKey[] = "user_id";
constexpr char kConsentStatusSettingsKey[] = "consent_status";
constexpr char kScriptVersionSettingsKey[] = "script_version";
constexpr char kUrlsToSkipSettingsKey[] = "urls_to_skip";

base::Value::List VectorToList(std::vector<std::string> values) {
  base::Value::List list;
  list.reserve(values.size());
  for (auto& value : values) {
    list.Append(std::move(value));
  }
  return list;
}

base::Value::Dict CreatePsstSettingsObject(PsstWebsiteSettings psst_metadata) {
  base::Value::Dict object;
  object.Set(kUserIdSettingsKey, psst_metadata.user_id);
  object.Set(kConsentStatusSettingsKey, ToString(psst_metadata.consent_status));
  object.Set(kScriptVersionSettingsKey, psst_metadata.script_version);
  object.Set(kUrlsToSkipSettingsKey,
             VectorToList(std::move(psst_metadata.urls_to_skip)));
  return object;
}

}  // namespace

PsstSettingsService::PsstSettingsService(
    HostContentSettingsMap& host_content_settings_map)
    : host_content_settings_map_(host_content_settings_map) {}

PsstSettingsService::~PsstSettingsService() = default;

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

void PsstSettingsService::SetPsstWebsiteSettings(
    const url::Origin& origin,
    ConsentStatus consent_status,
    int script_version,
    std::string_view user_id,
    base::Value::List urls_to_skip) {
  auto psst_metadata = PsstWebsiteSettings::FromValue(
      base::Value::Dict()
          .Set(kUserIdSettingsKey, user_id)
          .Set(kConsentStatusSettingsKey, ToString(consent_status))
          .Set(kScriptVersionSettingsKey, script_version)
          .Set(kUrlsToSkipSettingsKey, std::move(urls_to_skip)));
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
        base::Value(base::Value::Dict().Set(
            user_id, CreatePsstSettingsObject(std::move(psst_metadata)))));
  }
}

}  // namespace psst
