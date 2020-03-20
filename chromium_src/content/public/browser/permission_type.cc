/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/permission_type.h"

// Sadly, has to be a patch, otherwise:
// content/public/browser/permission_type.cc(36,11): error: enumeration value
// 'AUTOPLAY' not handled in switch [-Werror,-Wswitch]
// switch (descriptor->name) {
// ^
#define BRAVE_PERMISSIONDESCRIPTORTOPERMISSIONTYPE \
  case PermissionName::AUTOPLAY:                   \
    return PermissionType::AUTOPLAY;

#include "../../../../../content/public/browser/permission_type.cc"
#undef BRAVE_PERMISSIONDESCRIPTORTOPERMISSIONTYPE
