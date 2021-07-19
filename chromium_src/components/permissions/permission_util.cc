/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_util.h"

#define PermissionUtil PermissionUtil_ChromiumImpl
#include "../../../../components/permissions/permission_util.cc"
#undef PermissionUtil

namespace permissions {

// static
std::string PermissionUtil::GetPermissionString(
    ContentSettingsType content_type) {
  switch (content_type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return "BraveEthereum";
    default:
      return PermissionUtil_ChromiumImpl::GetPermissionString(content_type);
  }
}

// static
bool PermissionUtil::GetPermissionType(ContentSettingsType type,
                                       content::PermissionType* out) {
  if (type == ContentSettingsType::BRAVE_ETHEREUM) {
    *out = PermissionType::WINDOW_PLACEMENT;
    return true;
  }

  return PermissionUtil_ChromiumImpl::GetPermissionType(type, out);
}

// static
bool PermissionUtil::IsPermission(ContentSettingsType type) {
  switch (type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return true;
    default:
      return PermissionUtil_ChromiumImpl::IsPermission(type);
  }
}

}  // namespace permissions
