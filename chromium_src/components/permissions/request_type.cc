/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/request_type.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/resources/android/theme_resources.h"
#else
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/vector_icon_types.h"
#endif

#if BUILDFLAG(IS_ANDROID)
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
  case RequestType::kBraveSolana:             \
    return IDR_ANDROID_INFOBAR_PERMISSION_COOKIE

// Add Brave cases into GetIconIdDesktop.
#define kMicIcon                        \
  kMicIconValue;                        \
  case RequestType::kWidevine:          \
  case RequestType::kBraveEthereum:     \
  case RequestType::kBraveSolana:       \
  case RequestType::kBraveGoogleSignIn: \
    return vector_icons::kExtensionIcon

#define BRAVE_PERMISSION_KEY_FOR_REQUEST_TYPE        \
  case permissions::RequestType::kWidevine:          \
    return "widevine";                               \
  case permissions::RequestType::kBraveEthereum:     \
    return "brave_ethereum";                         \
  case permissions::RequestType::kBraveSolana:       \
    return "brave_solana";                           \
  case permissions::RequestType::kBraveGoogleSignIn: \
    return "brave_google_sign_in";

#define ContentSettingsTypeToRequestType \
  ContentSettingsTypeToRequestType_ChromiumImpl

#define RequestTypeToContentSettingsType \
  RequestTypeToContentSettingsType_ChromiumImpl

#define IsRequestablePermissionType IsRequestablePermissionType_ChromiumImpl

#include "src/components/permissions/request_type.cc"

#undef BRAVE_PERMISSION_KEY_FOR_REQUEST_TYPE
#undef IDR_ANDROID_INFOBAR_PERMISSION_COOKIE
#undef kMicIcon
#undef ContentSettingsTypeToRequestType
#undef RequestTypeToContentSettingsType
#undef IsRequestablePermissionType

namespace permissions {

RequestType ContentSettingsTypeToRequestType(
    ContentSettingsType content_settings_type) {
  switch (content_settings_type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return RequestType::kBraveEthereum;
    case ContentSettingsType::BRAVE_SOLANA:
      return RequestType::kBraveSolana;
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
      return RequestType::kBraveGoogleSignIn;
    default:
      return ContentSettingsTypeToRequestType_ChromiumImpl(
          content_settings_type);
  }
}
absl::optional<ContentSettingsType> RequestTypeToContentSettingsType(
    RequestType request_type) {
  switch (request_type) {
    case RequestType::kBraveEthereum:
      return ContentSettingsType::BRAVE_ETHEREUM;
    case RequestType::kBraveSolana:
      return ContentSettingsType::BRAVE_SOLANA;
    case RequestType::kBraveGoogleSignIn:
      return ContentSettingsType::BRAVE_GOOGLE_SIGN_IN;
    default:
      return RequestTypeToContentSettingsType_ChromiumImpl(request_type);
  }
}

bool IsRequestablePermissionType(ContentSettingsType content_settings_type) {
  return content_settings_type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN ||
         IsRequestablePermissionType_ChromiumImpl(content_settings_type);
}

}  // namespace permissions
