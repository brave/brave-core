/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "services/network/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#define BRAVE_WALLET_PERMISSION_UTIL_GET_PERMISSION_STRING \
  case PermissionType::BRAVE_ETHEREUM:                     \
    return "BraveEthereum";                                \
  case PermissionType::BRAVE_SOLANA:                       \
    return "BraveSolana";                                  \
  case PermissionType::BRAVE_CARDANO:                      \
    return "BraveCardano";
#else
#define BRAVE_WALLET_PERMISSION_UTIL_GET_PERMISSION_STRING \
  case PermissionType::BRAVE_ETHEREUM:                     \
  case PermissionType::BRAVE_SOLANA:                       \
  case PermissionType::BRAVE_CARDANO:                      \
    NOTREACHED();
#endif

#define PERMISSION_UTIL_GET_PERMISSION_STRING           \
  case PermissionType::BRAVE_ADS:                       \
    return "BraveAds";                                  \
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
    BRAVE_WALLET_PERMISSION_UTIL_GET_PERMISSION_STRING

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#define BRAVE_WALLET_K_DISPLAY_CAPTURE                          \
  case PermissionType::BRAVE_ETHEREUM:                          \
    return network::mojom::PermissionsPolicyFeature::kEthereum; \
  case PermissionType::BRAVE_SOLANA:                            \
    return network::mojom::PermissionsPolicyFeature::kSolana;   \
  case PermissionType::BRAVE_CARDANO:                           \
    return network::mojom::PermissionsPolicyFeature::kCardano;
#else
#define BRAVE_WALLET_K_DISPLAY_CAPTURE \
  case PermissionType::BRAVE_ETHEREUM: \
  case PermissionType::BRAVE_SOLANA:   \
  case PermissionType::BRAVE_CARDANO:  \
    return std::nullopt;
#endif

#define kDisplayCapture                                 \
  kDisplayCapture;                                      \
  BRAVE_WALLET_K_DISPLAY_CAPTURE                        \
  case PermissionType::BRAVE_ADS:                       \
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

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#define BRAVE_WALLET_PERMISSION_DESCRIPTOR_INFO_TO_PERMISSION_TYPE \
  case PermissionName::BRAVE_ETHEREUM:                             \
    return PermissionType::BRAVE_ETHEREUM;                         \
  case PermissionName::BRAVE_SOLANA:                               \
    return PermissionType::BRAVE_SOLANA;                           \
  case PermissionName::BRAVE_CARDANO:                              \
    return PermissionType::BRAVE_CARDANO;
#else
#define BRAVE_WALLET_PERMISSION_DESCRIPTOR_INFO_TO_PERMISSION_TYPE \
  case PermissionName::BRAVE_ETHEREUM:                             \
  case PermissionName::BRAVE_SOLANA:                               \
  case PermissionName::BRAVE_CARDANO:                              \
    NOTREACHED();
#endif

#define BRAVE_PERMISSION_UTIL_PERMISSION_DESCRIPTOR_INFO_TO_PERMISSION_TYPE \
  BRAVE_WALLET_PERMISSION_DESCRIPTOR_INFO_TO_PERMISSION_TYPE                \
  case PermissionName::BRAVE_ADS:                                           \
    return PermissionType::BRAVE_ADS;                                       \
  case PermissionName::BRAVE_TRACKERS:                                      \
    return PermissionType::BRAVE_TRACKERS;                                  \
  case PermissionName::BRAVE_HTTP_UPGRADABLE_RESOURCES:                     \
    return PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES;                 \
  case PermissionName::BRAVE_FINGERPRINTING_V2:                             \
    return PermissionType::BRAVE_FINGERPRINTING_V2;                         \
  case PermissionName::BRAVE_SHIELDS:                                       \
    return PermissionType::BRAVE_SHIELDS;                                   \
  case PermissionName::BRAVE_REFERRERS:                                     \
    return PermissionType::BRAVE_SHIELDS;                                   \
  case PermissionName::BRAVE_COOKIES:                                       \
    return PermissionType::BRAVE_COOKIES;                                   \
  case PermissionName::BRAVE_SPEEDREADER:                                   \
    return PermissionType::BRAVE_SPEEDREADER;                               \
  case PermissionName::BRAVE_GOOGLE_SIGN_IN:                                \
    return PermissionType::BRAVE_GOOGLE_SIGN_IN;                            \
  case PermissionName::BRAVE_LOCALHOST_ACCESS:                              \
    return PermissionType::BRAVE_LOCALHOST_ACCESS;                          \
  case PermissionName::BRAVE_OPEN_AI_CHAT:                                  \
    return PermissionType::BRAVE_OPEN_AI_CHAT;

#include <third_party/blink/common/permissions/permission_utils.cc>
#undef BRAVE_PERMISSION_UTIL_PERMISSION_DESCRIPTOR_INFO_TO_PERMISSION_TYPE
#undef BRAVE_WALLET_PERMISSION_DESCRIPTOR_INFO_TO_PERMISSION_TYPE
#undef kDisplayCapture
#undef BRAVE_WALLET_K_DISPLAY_CAPTURE
#undef PERMISSION_UTIL_GET_PERMISSION_STRING
#undef BRAVE_WALLET_PERMISSION_UTIL_GET_PERMISSION_STRING
