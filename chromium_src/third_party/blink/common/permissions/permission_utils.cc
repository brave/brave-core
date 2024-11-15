/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"

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
  case PermissionType::BRAVE_GOOGLE_SIGN_IN:            \
    return "BraveGoogleSignInPermission";               \
  case PermissionType::BRAVE_LOCALHOST_ACCESS:          \
    return "BraveLocalhostAccessPermission";            \
  case PermissionType::BRAVE_OPEN_AI_CHAT:              \
    return "BraveOpenAIChatPermission";                 \
  case PermissionType::BRAVE_ETHEREUM:                  \
    return "BraveEthereum";                             \
  case PermissionType::BRAVE_SOLANA:                    \
    return "BraveSolana";

#define kDisplayCapture                                 \
  kDisplayCapture;                                      \
  case PermissionType::BRAVE_ETHEREUM:                  \
    return mojom::PermissionsPolicyFeature::kEthereum;  \
  case PermissionType::BRAVE_SOLANA:                    \
    return mojom::PermissionsPolicyFeature::kSolana;    \
  case PermissionType::BRAVE_ADS:                       \
  case PermissionType::BRAVE_COSMETIC_FILTERING:        \
  case PermissionType::BRAVE_TRACKERS:                  \
  case PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES: \
  case PermissionType::BRAVE_FINGERPRINTING_V2:         \
  case PermissionType::BRAVE_SHIELDS:                   \
  case PermissionType::BRAVE_REFERRERS:                 \
  case PermissionType::BRAVE_COOKIES:                   \
  case PermissionType::BRAVE_SPEEDREADER:               \
  case PermissionType::BRAVE_GOOGLE_SIGN_IN:            \
  case PermissionType::BRAVE_LOCALHOST_ACCESS:          \
  case PermissionType::BRAVE_OPEN_AI_CHAT:              \
    return std::nullopt

#include "src/third_party/blink/common/permissions/permission_utils.cc"

#undef kDisplayCapture
#undef PERMISSION_UTIL_GET_PERMISSION_STRING
