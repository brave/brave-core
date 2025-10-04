/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/brave_psst_permission_context.h"

#include <optional>
#include <string_view>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/psst/common/psst_ui_common.mojom.h"
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

base::Value::Dict CreatePsstPermissionObject(
    PsstPermissionInfo psst_permission_info) {
  base::Value::Dict object;
  object.Set(kUserIdPermissionKey, psst_permission_info.user_id);
  object.Set(kConsentStatusPermissionKey,
             ToString(psst_permission_info.consent_status));
  object.Set(kScriptVersionPermissionKey, psst_permission_info.script_version);
  object.Set(kUrlsToSkipPermissionKey,
             VectorToList(std::move(psst_permission_info.urls_to_skip)));
  return object;
}

bool IsAllowedToProcess(const url::Origin& origin, std::string_view user_id) {
  return origin.scheme() == url::kHttpsScheme && !user_id.empty();
}

}  // namespace

BravePsstPermissionContext::BravePsstPermissionContext(
    HostContentSettingsMap* host_content_settings_map)
    : ObjectPermissionContextBase(ContentSettingsType::BRAVE_PSST,
                                  host_content_settings_map) {}
BravePsstPermissionContext::~BravePsstPermissionContext() = default;

std::string BravePsstPermissionContext::GetKeyForObject(
    const base::Value::Dict& object) {
  const auto* user_id = object.FindString(kUserIdPermissionKey);
  CHECK(user_id);
  return *user_id;
}

bool BravePsstPermissionContext::IsValidObject(
    const base::Value::Dict& object) {
  const auto* user_id = object.FindString(kUserIdPermissionKey);
  const auto* consent_status = object.FindString(kConsentStatusPermissionKey);
  const auto script_version = object.FindInt(kScriptVersionPermissionKey);
  const auto* urls_to_skip = object.FindList(kUrlsToSkipPermissionKey);

  return user_id && !user_id->empty() && consent_status &&
         !consent_status->empty() && script_version.has_value() && urls_to_skip;
}

std::u16string BravePsstPermissionContext::GetObjectDisplayName(
    const base::Value::Dict& object) {
  return base::UTF8ToUTF16(GetKeyForObject(object));
}

void BravePsstPermissionContext::GrantPermission(
    const url::Origin& origin,
    ConsentStatus consent_status,
    int script_version,
    std::string_view user_id,
    base::Value::List urls_to_skip) {
  auto permission_info = PsstPermissionInfo::FromValue(
      base::Value::Dict()
          .Set(kUserIdPermissionKey, user_id)
          .Set(kConsentStatusPermissionKey, static_cast<int>(consent_status))
          .Set(kScriptVersionPermissionKey, script_version)
          .Set(kUrlsToSkipPermissionKey, std::move(urls_to_skip)));
  if (!permission_info) {
    return;
  }

  GrantPermission(origin, std::move(permission_info.value()));
}

void BravePsstPermissionContext::GrantPermission(
    const url::Origin& origin,
    PsstPermissionInfo permission_info) {
  if (!IsAllowedToProcess(origin, permission_info.user_id)) {
    return;
  }

  if (const auto object = GetGrantedObject(origin, permission_info.user_id)) {
    UpdateObjectPermission(
        origin, object->value,
        CreatePsstPermissionObject(std::move(permission_info)));
    return;
  }

  GrantObjectPermission(origin,
                        CreatePsstPermissionObject(std::move(permission_info)));
}

bool BravePsstPermissionContext::HasPermission(const url::Origin& origin,
                                               std::string_view user_id) {
  return !!GetPsstPermissionInfo(origin, user_id);
}

void BravePsstPermissionContext::RevokePermission(const url::Origin& origin,
                                                  std::string_view user_id) {
  if (!IsAllowedToProcess(origin, user_id)) {
    return;
  }

  RevokeObjectPermission(origin, user_id);
}

std::optional<PsstPermissionInfo>
BravePsstPermissionContext::GetPsstPermissionInfo(const url::Origin& origin,
                                                  std::string_view user_id) {
  if (!IsAllowedToProcess(origin, user_id)) {
    return std::nullopt;
  }

  const auto object = GetGrantedObject(origin, user_id);
  if (!object) {
    return std::nullopt;
  }

  return PsstPermissionInfo::FromValue(object->value);
}

}  // namespace psst
