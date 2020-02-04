/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/permission_util.h"

#define GetPermissionString GetPermissionString_ChromiumImpl
#define GetRequestType GetRequestType_ChromiumImpl
#define GetPermissionType GetPermissionType_ChromiumImpl
#define IsPermission IsPermission_ChromiumImpl
#include "../../../../../../chrome/browser/permissions/permission_util.cc"
#undef IsPermission
#undef GetPermissionType
#undef GetRequestType
#undef GetPermissionString

std::string PermissionUtil::GetPermissionString(
    ContentSettingsType content_type) {
  if (content_type == ContentSettingsType::AUTOPLAY)
    return "Autoplay";
  return GetPermissionString_ChromiumImpl(content_type);
}

PermissionRequestType PermissionUtil::GetRequestType(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY) {
    return PermissionRequestType::PERMISSION_AUTOPLAY;
  }
  return GetRequestType_ChromiumImpl(type);
}

bool PermissionUtil::GetPermissionType(ContentSettingsType type,
                                       PermissionType* out) {
  if (type == ContentSettingsType::AUTOPLAY) {
    *out = PermissionType::AUTOPLAY;
    return true;
  }
  return GetPermissionType_ChromiumImpl(type, out);
}

bool PermissionUtil::IsPermission(ContentSettingsType type) {
  if (type == ContentSettingsType::AUTOPLAY)
    return true;
  return IsPermission_ChromiumImpl(type);
}
