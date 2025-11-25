/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_PERMISSIONS_PERMISSION_UTILS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_PERMISSIONS_PERMISSION_UTILS_H_

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

// clang-format off
#define NUM                         \
  BRAVE_ADS,                        \
  BRAVE_TRACKERS,                   \
  BRAVE_HTTP_UPGRADABLE_RESOURCES,  \
  BRAVE_FINGERPRINTING_V2,          \
  BRAVE_SHIELDS,                    \
  BRAVE_REFERRERS,                  \
  BRAVE_COOKIES,                    \
  BRAVE_SPEEDREADER,                \
  BRAVE_ETHEREUM,                   \
  BRAVE_SOLANA,                     \
  BRAVE_GOOGLE_SIGN_IN,             \
  BRAVE_LOCALHOST_ACCESS,           \
  BRAVE_OPEN_AI_CHAT,               \
  BRAVE_CARDANO,                    \
  NUM
// clang-format on

#include <third_party/blink/public/common/permissions/permission_utils.h>  // IWYU pragma: export
#undef NUM

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_PERMISSIONS_PERMISSION_UTILS_H_
