/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "components/permissions/permission_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#define BRAVE_WALLET_PERMISSION_TYPES  \
  case PermissionType::BRAVE_ETHEREUM: \
  case PermissionType::BRAVE_SOLANA:   \
  case PermissionType::BRAVE_CARDANO:
#else
#define BRAVE_WALLET_PERMISSION_TYPES
#endif

#define NUM                                             \
  BRAVE_ADS:                                            \
  case PermissionType::BRAVE_TRACKERS:                  \
  case PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES: \
  case PermissionType::BRAVE_FINGERPRINTING_V2:         \
  case PermissionType::BRAVE_SHIELDS:                   \
  case PermissionType::BRAVE_REFERRERS:                 \
  case PermissionType::BRAVE_COOKIES:                   \
  case PermissionType::BRAVE_SPEEDREADER:               \
  BRAVE_WALLET_PERMISSION_TYPES                         \
  case PermissionType::BRAVE_GOOGLE_SIGN_IN:            \
  case PermissionType::BRAVE_LOCALHOST_ACCESS:          \
  case PermissionType::BRAVE_OPEN_AI_CHAT:              \
  case PermissionType::NUM

#include <content/shell/browser/shell_permission_manager.cc>
#undef NUM
#undef BRAVE_WALLET_PERMISSION_TYPES
