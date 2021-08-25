/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/request_type.h"

#if defined(OS_ANDROID)
#include "components/resources/android/theme_resources.h"
#else
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/vector_icon_types.h"
#endif

#if defined(OS_ANDROID)
namespace {
constexpr auto kAndroidInfobarPermissionCookie =
    IDR_ANDROID_INFOBAR_PERMISSION_COOKIE;
}  // namespace
#else
namespace vector_icons {
constexpr auto& kMicIconValue = vector_icons::kMicIcon;
}  // namespace vector_icons
#endif

// Add Brave cases into GetIconIdAndroid.
// kWidevine is not expected to happen here as Widevine is not enabled in
// Android, we add this case here just to avoid build error due to unhandled
// cases in the switch.
//
// TODO(jocelyn): Might need to update icon when we have ethereum.enable UI
// support in Android.
#define IDR_ANDROID_INFOBAR_PERMISSION_COOKIE \
  kAndroidInfobarPermissionCookie;            \
  case RequestType::kWidevine:                \
  case RequestType::kBraveEthereum:           \
    return IDR_ANDROID_INFOBAR_PERMISSION_COOKIE

// Add Brave cases into GetIconIdDesktop.
#define kMicIcon                    \
  kMicIconValue;                    \
  case RequestType::kWidevine:      \
  case RequestType::kBraveEthereum: \
    return vector_icons::kExtensionIcon

#define BRAVE_PERMISSION_KEY_FOR_REQUEST_TYPE    \
  case permissions::RequestType::kWidevine:      \
    return "widevine";                           \
  case permissions::RequestType::kBraveEthereum: \
    return "brave_ethereum";

#define ContentSettingsTypeToRequestType \
  ContentSettingsTypeToRequestType_ChromiumImpl

#include "../../../../components/permissions/request_type.cc"

#undef BRAVE_PERMISSION_KEY_FOR_REQUEST_TYPE
#undef IDR_ANDROID_INFOBAR_PERMISSION_COOKIE
#undef kMicIcon
#undef ContentSettingsTypeToRequestType

namespace permissions {

RequestType ContentSettingsTypeToRequestType(
    ContentSettingsType content_settings_type) {
  switch (content_settings_type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return RequestType::kBraveEthereum;
    default:
      return ContentSettingsTypeToRequestType_ChromiumImpl(
          content_settings_type);
  }
}

}  // namespace permissions
