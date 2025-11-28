/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/request_type.h"

#include <optional>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/resources/android/theme_resources.h"
#else
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/vector_icon_types.h"
#endif

#if BUILDFLAG(IS_ANDROID)
namespace {
constexpr auto kAndroidStorageAccess = IDR_ANDROID_STORAGE_ACCESS;
}  // namespace
#endif

// Add Brave cases into GetIconIdAndroid.
// TODO(jocelyn): Might need to update icon when we have ethereum.enable UI
// support in Android.
#define BRAVE_WALLET_ANDROID_CASES  \
  case RequestType::kBraveEthereum: \
  case RequestType::kBraveSolana:   \
  case RequestType::kBraveCardano:

#define IDR_ANDROID_STORAGE_ACCESS                   \
  kAndroidStorageAccess;                             \
  case RequestType::kWidevine:                       \
    BRAVE_WALLET_ANDROID_CASES                       \
  case RequestType::kBraveGoogleSignInPermission:    \
  case RequestType::kBraveLocalhostAccessPermission: \
  case RequestType::kBraveOpenAIChat:                \
    return IDR_ANDROID_INFOBAR_PERMISSION_COOKIE

// Add Brave cases into GetIconIdDesktop.
#define BRAVE_WALLET_DESKTOP_CASES  \
  case RequestType::kBraveEthereum: \
  case RequestType::kBraveSolana:   \
  case RequestType::kBraveCardano:

#define kStorageAccessIcon                           \
  kStorageAccessIcon;                                \
  case RequestType::kWidevine:                       \
    BRAVE_WALLET_DESKTOP_CASES                       \
  case RequestType::kBraveGoogleSignInPermission:    \
  case RequestType::kBraveLocalhostAccessPermission: \
  case RequestType::kBraveOpenAIChat:                \
    return vector_icons::kExtensionIcon

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#define BRAVE_WALLET_PERMISSION_KEYS             \
  case permissions::RequestType::kBraveEthereum: \
    return "brave_ethereum";                     \
  case permissions::RequestType::kBraveSolana:   \
    return "brave_solana";                       \
  case permissions::RequestType::kBraveCardano:  \
    return "brave_cardano";
#else
#define BRAVE_WALLET_PERMISSION_KEYS             \
  case permissions::RequestType::kBraveEthereum: \
  case permissions::RequestType::kBraveSolana:   \
  case permissions::RequestType::kBraveCardano:  \
    NOTREACHED();
#endif

#define BRAVE_PERMISSION_KEY_FOR_REQUEST_TYPE                     \
  case permissions::RequestType::kWidevine:                       \
    return "widevine";                                            \
    BRAVE_WALLET_PERMISSION_KEYS                                  \
  case permissions::RequestType::kBraveGoogleSignInPermission:    \
    return "brave_google_sign_in";                                \
  case permissions::RequestType::kBraveLocalhostAccessPermission: \
    return "brave_localhost_access";                              \
  case permissions::RequestType::kBraveOpenAIChat:                \
    return "brave_ai_chat";

#define ContentSettingsTypeToRequestType \
  ContentSettingsTypeToRequestType_ChromiumImpl

#define RequestTypeToContentSettingsType \
  RequestTypeToContentSettingsType_ChromiumImpl

#define IsRequestablePermissionType IsRequestablePermissionType_ChromiumImpl

#include <components/permissions/request_type.cc>

#undef IsRequestablePermissionType
#undef RequestTypeToContentSettingsType
#undef ContentSettingsTypeToRequestType
#undef BRAVE_PERMISSION_KEY_FOR_REQUEST_TYPE
#undef BRAVE_WALLET_PERMISSION_KEYS
#undef kStorageAccessIcon
#undef BRAVE_WALLET_DESKTOP_CASES
#undef IDR_ANDROID_STORAGE_ACCESS
#undef BRAVE_WALLET_ANDROID_CASES

namespace permissions {

RequestType ContentSettingsTypeToRequestType(
    ContentSettingsType content_settings_type) {
  switch (content_settings_type) {
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    case ContentSettingsType::BRAVE_ETHEREUM:
      return RequestType::kBraveEthereum;
    case ContentSettingsType::BRAVE_SOLANA:
      return RequestType::kBraveSolana;
    case ContentSettingsType::BRAVE_CARDANO:
      return RequestType::kBraveCardano;
#endif
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
      return RequestType::kBraveGoogleSignInPermission;
    case ContentSettingsType::BRAVE_LOCALHOST_ACCESS:
      return RequestType::kBraveLocalhostAccessPermission;
    case ContentSettingsType::BRAVE_OPEN_AI_CHAT:
      return RequestType::kBraveOpenAIChat;
    case ContentSettingsType::DEFAULT:
      // Currently we have only one DEFAULT type that is
      // not mapped, which is Widevine, it's used for
      // UMA purpose only
      return RequestType::kWidevine;
    default:
      return ContentSettingsTypeToRequestType_ChromiumImpl(
          content_settings_type);
  }
}

std::optional<ContentSettingsType> RequestTypeToContentSettingsType(
    RequestType request_type) {
  switch (request_type) {
    case RequestType::kBraveGoogleSignInPermission:
      return ContentSettingsType::BRAVE_GOOGLE_SIGN_IN;
    case RequestType::kBraveLocalhostAccessPermission:
      return ContentSettingsType::BRAVE_LOCALHOST_ACCESS;
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    case RequestType::kBraveEthereum:
      return ContentSettingsType::BRAVE_ETHEREUM;
    case RequestType::kBraveSolana:
      return ContentSettingsType::BRAVE_SOLANA;
    case RequestType::kBraveCardano:
      return ContentSettingsType::BRAVE_CARDANO;
#endif
    case RequestType::kBraveOpenAIChat:
      return ContentSettingsType::BRAVE_OPEN_AI_CHAT;
    default:
      return RequestTypeToContentSettingsType_ChromiumImpl(request_type);
  }
}

bool IsRequestablePermissionType(ContentSettingsType content_settings_type) {
  switch (content_settings_type) {
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
    case ContentSettingsType::BRAVE_LOCALHOST_ACCESS:
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    case ContentSettingsType::BRAVE_ETHEREUM:
    case ContentSettingsType::BRAVE_SOLANA:
    case ContentSettingsType::BRAVE_CARDANO:
#endif
    case ContentSettingsType::BRAVE_OPEN_AI_CHAT:
      return true;
    default:
      return IsRequestablePermissionType_ChromiumImpl(content_settings_type);
  }
}

}  // namespace permissions
