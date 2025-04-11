/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/permissions/permission_utils.h"

#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

#define WEB_PRINTING                                    \
  BRAVE_ADS:                                            \
  return "brave_ads";                                   \
  case PermissionName::BRAVE_COSMETIC_FILTERING:        \
    return "brave_cosmetic_filtering";                  \
  case PermissionName::BRAVE_TRACKERS:                  \
    return "brave_trackers";                            \
  case PermissionName::BRAVE_HTTP_UPGRADABLE_RESOURCES: \
    return "brave_http_upgradable_resources";           \
  case PermissionName::BRAVE_FINGERPRINTING_V2:         \
    return "brave_fingerprinting_v2";                   \
  case PermissionName::BRAVE_SHIELDS:                   \
    return "brave_shields";                             \
  case PermissionName::BRAVE_REFERRERS:                 \
    return "brave_referrers";                           \
  case PermissionName::BRAVE_COOKIES:                   \
    return "brave_cookies";                             \
  case PermissionName::BRAVE_SPEEDREADER:               \
    return "brave_speedreader";                         \
  case PermissionName::BRAVE_ETHEREUM:                  \
    return "brave_ethereum";                            \
  case PermissionName::BRAVE_SOLANA:                    \
    return "brave_solana";                              \
  case PermissionName::BRAVE_GOOGLE_SIGN_IN:            \
    return "brave_google_sign_in";                      \
  case PermissionName::BRAVE_LOCALHOST_ACCESS:          \
    return "brave_localhost_access";                    \
  case PermissionName::BRAVE_OPEN_AI_CHAT:              \
    return "brave_open_ai_chat";                        \
  case PermissionName::WEB_PRINTING

#include "src/third_party/blink/renderer/modules/permissions/permission_utils.cc"

#undef WEB_PRINTING
