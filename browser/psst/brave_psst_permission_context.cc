/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/brave_psst_permission_context.h"

#include <optional>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace psst {

namespace {

constexpr char kSignedUserIdPermissionKey[] = "signed_user_id";
constexpr char kConsentStatusPermissionKey[] = "consent_status";
constexpr char kScriptVersionPermissionKey[] = "script_version";
constexpr char kUrlsToSkipPermissionKey[] = "urls_to_skip";

base::Value::Dict CreatePsstPermissionObject(
    const PsstPermissionInfo& psst_permission_info) {
  base::Value::Dict object;
  object.Set(kSignedUserIdPermissionKey, psst_permission_info.user_id);
  object.Set(kConsentStatusPermissionKey,
             static_cast<int>(psst_permission_info.consent_status));
  object.Set(kScriptVersionPermissionKey, psst_permission_info.script_version);
  object.Set(kUrlsToSkipPermissionKey,
             psst_permission_info.urls_to_skip.Clone());
  return object;
}

bool IsAllowedToProcess(const url::Origin& origin, const std::string& user_id) {
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
  const auto* user_id = object.FindString(kSignedUserIdPermissionKey);
  CHECK(user_id);
  return *user_id;
}

bool BravePsstPermissionContext::IsValidObject(
    const base::Value::Dict& object) {
  const auto* user_id = object.FindString(kSignedUserIdPermissionKey);
  const auto consent_status = object.FindInt(kConsentStatusPermissionKey);
  const auto script_version = object.FindInt(kScriptVersionPermissionKey);
  const auto* urls_to_skip = object.FindList(kUrlsToSkipPermissionKey);

  return user_id && !user_id->empty() && consent_status &&
         script_version.has_value() && urls_to_skip;
}

std::u16string BravePsstPermissionContext::GetObjectDisplayName(
    const base::Value::Dict& object) {
  return base::UTF8ToUTF16(GetKeyForObject(object));
}

void BravePsstPermissionContext::CreateOrUpdate(
    const url::Origin& origin,
    const PsstPermissionInfo& permission_info) {
  if (!IsAllowedToProcess(origin, permission_info.user_id)) {
    return;
  }

  if (const auto object = GetGrantedObject(origin, permission_info.user_id)) {
    UpdateObjectPermission(origin, object->value,
                           CreatePsstPermissionObject(permission_info));
    return;
  }

  GrantObjectPermission(origin, CreatePsstPermissionObject(permission_info));

  FlushScheduledSaveSettingsCalls();
}

void BravePsstPermissionContext::Revoke(const url::Origin& origin,
                                        const std::string& user_id) {
  if (!IsAllowedToProcess(origin, user_id)) {
    return;
  }

  RevokeObjectPermission(origin, user_id);
}

std::optional<PsstPermissionInfo>
BravePsstPermissionContext::GetPsstPermissionInfo(const url::Origin& origin,
                                                  const std::string& user_id) {
  if (!IsAllowedToProcess(origin, user_id)) {
    return std::nullopt;
  }

  const auto object = GetGrantedObject(origin, user_id);
  if (!object) {
    return std::nullopt;
  }

  const base::Value::List* urls =
      object->value.FindList(kUrlsToSkipPermissionKey);
  return PsstPermissionInfo{
      static_cast<psst::ConsentStatus>(
          object->value.FindInt(kConsentStatusPermissionKey).value()),
      object->value.FindInt(kScriptVersionPermissionKey).value(), user_id,
      urls->Clone()};
}

}  // namespace psst
