/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/browser/permissions/permission_controller_impl.h"
#include "content/browser/permissions/permission_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#define NUM                                             \
  BRAVE_ADS:                                            \
  case PermissionType::BRAVE_COSMETIC_FILTERING:        \
  case PermissionType::BRAVE_TRACKERS:                  \
  case PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES: \
  case PermissionType::BRAVE_FINGERPRINTING_V2:         \
  case PermissionType::BRAVE_SHIELDS:                   \
  case PermissionType::BRAVE_REFERRERS:                 \
  case PermissionType::BRAVE_COOKIES:                   \
  case PermissionType::BRAVE_SPEEDREADER:               \
  case PermissionType::BRAVE_ETHEREUM:                  \
  case PermissionType::BRAVE_SOLANA:                    \
  case PermissionType::BRAVE_GOOGLE_SIGN_IN:            \
  case PermissionType::BRAVE_LOCALHOST_ACCESS:          \
  case PermissionType::NUM

#include "src/content/browser/permissions/permission_controller_impl.cc"
#undef NUM
