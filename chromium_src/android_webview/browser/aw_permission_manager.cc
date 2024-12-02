/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "android_webview/browser/aw_permission_manager.h"

#include "components/permissions/permission_util.h"
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
  case PermissionType::BRAVE_OPEN_AI_CHAT:              \
  case PermissionType::NUM

namespace android_webview {

void AwPermissionManager::SetOriginCanReadEnumerateDevicesAudioLabels(
    const url::Origin& origin,
    bool audio) {}

void AwPermissionManager::SetOriginCanReadEnumerateDevicesVideoLabels(
    const url::Origin& origin,
    bool video) {}

}  // namespace android_webview

#define SetOriginCanReadEnumerateDevicesAudioLabels \
  SetOriginCanReadEnumerateDevicesAudioLabels_ChromiumImpl
#define SetOriginCanReadEnumerateDevicesVideoLabels \
  SetOriginCanReadEnumerateDevicesVideoLabels_ChromiumImpl

#include "src/android_webview/browser/aw_permission_manager.cc"

#undef SetOriginCanReadEnumerateDevicesAudioLabels
#undef SetOriginCanReadEnumerateDevicesVideoLabels
#undef NUM
