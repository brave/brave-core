/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_util.h"

#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_request.h"

namespace permissions {
namespace {

std::string GetPermissionRequestString_ChromiumImpl(PermissionRequestType type);
void BraveRecordPermissionAction(ContentSettingsType permission,
                                 bool secure_origin,
                                 PermissionAction action);

std::string GetPermissionRequestString(PermissionRequestType type) {
  if (type == PermissionRequestType::PERMISSION_AUTOPLAY)
    return "Autoplay";
  if (type == PermissionRequestType::PERMISSION_WIDEVINE)
    return "Widevine";
  if (type == PermissionRequestType::PERMISSION_WALLET)
    return "Wallet";
  return GetPermissionRequestString_ChromiumImpl(type);
}

}  // namespace
}  // namespace permissions

#define BRAVE_PERMISSIONUMAUTIL_RECORDPERMISSIONACTION              \
  case ContentSettingsType::AUTOPLAY:                               \
    BraveRecordPermissionAction(permission, secure_origin, action); \
    break; \

#include "../../../../components/permissions/permission_uma_util.cc"
#undef BRAVE_PERMISSIONUMAUTIL_RECORDPERMISSIONACTION

namespace permissions {
namespace {

void BraveRecordPermissionAction(ContentSettingsType permission,
                                 bool secure_origin,
                                 PermissionAction action) {
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
}  // namespace permissions
