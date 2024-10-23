/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_UTIL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_UTIL_H_

#define PermissionUtil PermissionUtil_ChromiumImpl
#include "src/components/permissions/permission_util.h"  // IWYU pragma: export
#undef PermissionUtil

namespace permissions {

class PermissionUtil : public PermissionUtil_ChromiumImpl {
 public:
  static std::string GetPermissionString(ContentSettingsType);
  static bool GetPermissionType(ContentSettingsType type,
                                blink::PermissionType* out);
  static bool IsPermission(ContentSettingsType type);

  static blink::PermissionType ContentSettingsTypeToPermissionType(
      ContentSettingsType permission);

  static GURL GetCanonicalOrigin(ContentSettingsType permission,
                                 const GURL& requesting_origin,
                                 const GURL& embedding_origin);
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_UTIL_H_
