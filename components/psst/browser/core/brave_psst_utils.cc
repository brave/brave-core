/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/brave_psst_utils.h"

#include <optional>
#include <string_view>

#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/psst/common/psst_metadata_schema.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace psst {

namespace {

constexpr char kUserIdPermissionKey[] = "user_id";
constexpr char kConsentStatusPermissionKey[] = "consent_status";
constexpr char kScriptVersionPermissionKey[] = "script_version";
constexpr char kUrlsToSkipPermissionKey[] = "urls_to_skip";

base::Value::List VectorToList(std::vector<std::string> values) {
  base::Value::List list;
  list.reserve(values.size());
  for (auto& value : values) {
    list.Append(std::move(value));
  }
  return list;
}

base::Value::Dict CreatePsstPermissionObject(PsstMetadata psst_metadata) {
  base::Value::Dict object;
  object.Set(kUserIdPermissionKey, psst_metadata.user_id);
  object.Set(kConsentStatusPermissionKey,
             ToString(psst_metadata.consent_status));
  object.Set(kScriptVersionPermissionKey, psst_metadata.script_version);
  object.Set(kUrlsToSkipPermissionKey,
             VectorToList(std::move(psst_metadata.urls_to_skip)));
  return object;
}

}  // namespace

std::optional<PsstMetadata> GetPsstMetadata(HostContentSettingsMap* map,
                                            const url::Origin& origin,
                                            std::string_view user_id) {
  auto metadata_objects = map->GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);
  auto* metadata_objects_dict = metadata_objects.GetIfDict();
  if (!metadata_objects_dict) {
    return std::nullopt;
  }

  auto* user_id_metadata_dict = metadata_objects_dict->FindDict(user_id);
  if (!user_id_metadata_dict) {
    return std::nullopt;
  }

  return PsstMetadata::FromValue(*user_id_metadata_dict);
}

void SetPsstMetadata(HostContentSettingsMap* map,
                     const url::Origin& origin,
                     ConsentStatus consent_status,
                     int script_version,
                     std::string_view user_id,
                     base::Value::List urls_to_skip) {
  auto psst_metadata = PsstMetadata::FromValue(
      base::Value::Dict()
          .Set(kUserIdPermissionKey, user_id)
          .Set(kConsentStatusPermissionKey, ToString(consent_status))
          .Set(kScriptVersionPermissionKey, script_version)
          .Set(kUrlsToSkipPermissionKey, std::move(urls_to_skip)));
  if (!psst_metadata) {
    return;
  }

  SetPsstMetadata(map, origin, std::move(*psst_metadata));
}

void SetPsstMetadata(HostContentSettingsMap* map,
                     const url::Origin& origin,
                     PsstMetadata psst_metadata) {
  if (origin.scheme() != url::kHttpsScheme) {
    return;
  }

  auto metadata_objects = map->GetWebsiteSetting(
      origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST);

  const auto user_id = psst_metadata.user_id;
  auto* metadata_objects_dict = metadata_objects.GetIfDict();
  if (metadata_objects_dict) {
    metadata_objects_dict->Set(
        user_id, CreatePsstPermissionObject(std::move(psst_metadata)));
    map->SetWebsiteSettingDefaultScope(
        origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST,
        base::Value(std::move(*metadata_objects_dict)));
  } else {
    map->SetWebsiteSettingDefaultScope(
        origin.GetURL(), origin.GetURL(), ContentSettingsType::BRAVE_PSST,
        base::Value(base::Value::Dict().Set(
            user_id, CreatePsstPermissionObject(std::move(psst_metadata)))));
  }
}

}  // namespace psst
