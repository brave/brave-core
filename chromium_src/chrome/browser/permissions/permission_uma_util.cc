/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/permission_util.h"

#include "chrome/browser/permissions/permission_request.h"
#include "components/content_settings/core/common/content_settings_types.h"

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

#include "../../../../../chrome/browser/permissions/permission_uma_util.cc"  // NOLINT

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
