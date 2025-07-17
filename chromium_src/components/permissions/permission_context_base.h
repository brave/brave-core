/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_

namespace permissions {
class ContentSettingPermissionContextBase;
}  // namespace permissions

#define PermissionContextBaseTests \
  PermissionContextBaseTests;      \
  friend ContentSettingPermissionContextBase

#define PermissionDecided virtual PermissionDecided
#define CleanUpRequest virtual CleanUpRequest

#include "src/components/permissions/permission_context_base.h"  // IWYU pragma: export

#undef PermissionContextBaseTests
#undef PermissionDecided
#undef CleanUpRequest

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_CONTEXT_BASE_H_
