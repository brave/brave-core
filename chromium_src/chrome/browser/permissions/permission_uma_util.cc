/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_util.h"

#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request.h"

namespace {

std::string GetPermissionRequestString_ChromiumImpl(
    permissions::PermissionRequestType type);
void BraveRecordPermissionAction(ContentSettingsType permission,
                                 bool secure_origin,
                                 permissions::PermissionAction action);

std::string GetPermissionRequestString(
    permissions::PermissionRequestType type) {
  if (type == permissions::PermissionRequestType::PERMISSION_AUTOPLAY)
    return "Autoplay";
  if (type == permissions::PermissionRequestType::PERMISSION_WIDEVINE)
    return "Widevine";
  if (type == permissions::PermissionRequestType::PERMISSION_WALLET)
    return "Wallet";
  return GetPermissionRequestString_ChromiumImpl(type);
}

}  // namespace

#define BRAVE_PERMISSIONUMAUTIL_RECORDPERMISSIONACTION              \
  case ContentSettingsType::AUTOPLAY:                               \
    BraveRecordPermissionAction(permission, secure_origin, action); \
    break; \

#include "../../../../../chrome/browser/permissions/permission_uma_util.cc"  // NOLINT
#undef BRAVE_PERMISSIONUMAUTIL_RECORDPERMISSIONACTION

namespace {

void BraveRecordPermissionAction(ContentSettingsType permission,
                                 bool secure_origin,
                                 permissions::PermissionAction action) {
  switch (permission) {
    case ContentSettingsType::AUTOPLAY:
      PERMISSION_ACTION_UMA(secure_origin, "Permissions.Action.Autoplay",
                            "Permissions.Action.SecureOrigin.Autoplay",
                            "Permissions.Action.InsecureOrigin.Autoplay",
                            action);
      break;
    default:
      break;
  }
}

}  // namespace
