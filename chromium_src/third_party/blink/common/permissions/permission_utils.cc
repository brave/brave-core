/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define PERMISSION_UTIL_GET_PERMISSION_STRING           \
  case PermissionType::BRAVE_ADS:                       \
    return "BraveAds";                                  \
  case PermissionType::BRAVE_COSMETIC_FILTERING:        \
    return "BraveCosmeticFiltering";                    \
  case PermissionType::BRAVE_TRACKERS:                  \
    return "BraveTrackers";                             \
  case PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES: \
    return "BraveHttpUpgradableResource";               \
  case PermissionType::BRAVE_FINGERPRINTING_V2:         \
    return "BraveFingerprintingV2";                     \
  case PermissionType::BRAVE_SHIELDS:                   \
    return "BraveShields";                              \
  case PermissionType::BRAVE_REFERRERS:                 \
    return "BraveReferrers";                            \
  case PermissionType::BRAVE_COOKIES:                   \
    return "BraveCookies";                              \
  case PermissionType::BRAVE_SPEEDREADER:               \
    return "BraveSpeedreaders";                         \
  case PermissionType::BRAVE_ETHEREUM:                  \
    return "BraveEthereum";                             \
  case PermissionType::BRAVE_SOLANA:                    \
    return "BraveSolana";

#include "src/third_party/blink/common/permissions/permission_utils.cc"

#undef PERMISSION_UTIL_GET_PERMISSION_STRING
