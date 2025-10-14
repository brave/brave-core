/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_util.h"

#include "components/permissions/permission_uma_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#define PermissionUtil PermissionUtil_ChromiumImpl

// Since we don't do UMA just reuse an existing UMA type instead of adding one.
#define BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE         \
  case RequestType::kWidevine:                       \
  case RequestType::kBraveEthereum:                  \
  case RequestType::kBraveSolana:                    \
  case RequestType::kBraveCardano:                   \
  case RequestType::kBraveGoogleSignInPermission:    \
  case RequestType::kBraveLocalhostAccessPermission: \
  case RequestType::kBraveOpenAIChat:                \
  case RequestType::kBravePsst:                      \
    return RequestTypeForUma::PERMISSION_VR;

// These requests may be batched together, so we must handle them explicitly as
// GetUmaValueForRequests expects only a few specific request types to be
// batched
#define BRAVE_GET_UMA_VALUE_FOR_REQUESTS             \
  if (request_type >= RequestType::kBraveMinValue && \
      request_type <= RequestType::kBraveMaxValue) { \
    return GetUmaValueForRequestType(request_type);  \
  }

#define PERMISSION_UTIL_PERMISSION_TYPE_TO_CONTENT_SETTINGS_TYPE \
  case PermissionType::BRAVE_ADS:                                \
    return ContentSettingsType::BRAVE_ADS;                       \
  case PermissionType::BRAVE_TRACKERS:                           \
    return ContentSettingsType::BRAVE_TRACKERS;                  \
  case PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES:          \
    return ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES; \
  case PermissionType::BRAVE_FINGERPRINTING_V2:                  \
    return ContentSettingsType::BRAVE_FINGERPRINTING_V2;         \
  case PermissionType::BRAVE_SHIELDS:                            \
    return ContentSettingsType::BRAVE_SHIELDS;                   \
  case PermissionType::BRAVE_REFERRERS:                          \
    return ContentSettingsType::BRAVE_REFERRERS;                 \
  case PermissionType::BRAVE_COOKIES:                            \
    return ContentSettingsType::BRAVE_COOKIES;                   \
  case PermissionType::BRAVE_SPEEDREADER:                        \
    return ContentSettingsType::BRAVE_SPEEDREADER;               \
  case PermissionType::BRAVE_ETHEREUM:                           \
    return ContentSettingsType::BRAVE_ETHEREUM;                  \
  case PermissionType::BRAVE_SOLANA:                             \
    return ContentSettingsType::BRAVE_SOLANA;                    \
  case PermissionType::BRAVE_GOOGLE_SIGN_IN:                     \
    return ContentSettingsType::BRAVE_GOOGLE_SIGN_IN;            \
  case PermissionType::BRAVE_LOCALHOST_ACCESS:                   \
    return ContentSettingsType::BRAVE_LOCALHOST_ACCESS;          \
  case PermissionType::BRAVE_OPEN_AI_CHAT:                       \
    return ContentSettingsType::BRAVE_OPEN_AI_CHAT;              \
  case PermissionType::BRAVE_CARDANO:                            \
    return ContentSettingsType::BRAVE_CARDANO;

#include <components/permissions/permission_util.cc>
#undef PermissionUtil
#undef BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE
#undef BRAVE_GET_UMA_VALUE_FOR_REQUESTS
#undef PERMISSION_UTIL_PERMISSION_TYPE_TO_CONTENT_SETTINGS_TYPE

namespace permissions {

// static
std::string PermissionUtil::GetPermissionString(
    ContentSettingsType content_type) {
  switch (content_type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return "BraveEthereum";
    case ContentSettingsType::BRAVE_SOLANA:
      return "BraveSolana";
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
      return "BraveGoogleSignInPermission";
    case ContentSettingsType::BRAVE_LOCALHOST_ACCESS:
      return "BraveLocalhostAccessPermission";
    case ContentSettingsType::BRAVE_OPEN_AI_CHAT:
      return "BraveOpenAIChatPermission";
    case ContentSettingsType::BRAVE_CARDANO:
      return "BraveCardano";
    default:
      return PermissionUtil_ChromiumImpl::GetPermissionString(content_type);
  }
}

// static
bool PermissionUtil::GetPermissionType(ContentSettingsType type,
                                       blink::PermissionType* out) {
  if (type == ContentSettingsType::BRAVE_ETHEREUM ||
      type == ContentSettingsType::BRAVE_SOLANA ||
      type == ContentSettingsType::BRAVE_CARDANO) {
    *out = PermissionType::WINDOW_MANAGEMENT;
    return true;
  }
  if (type == ContentSettingsType::BRAVE_GOOGLE_SIGN_IN) {
    *out = PermissionType::BRAVE_GOOGLE_SIGN_IN;
    return true;
  }
  if (type == ContentSettingsType::BRAVE_LOCALHOST_ACCESS) {
    *out = PermissionType::BRAVE_LOCALHOST_ACCESS;
    return true;
  }
  if (type == ContentSettingsType::BRAVE_OPEN_AI_CHAT) {
    *out = PermissionType::BRAVE_OPEN_AI_CHAT;
    return true;
  }

  return PermissionUtil_ChromiumImpl::GetPermissionType(type, out);
}

// static
bool PermissionUtil::IsPermission(ContentSettingsType type) {
  switch (type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
    case ContentSettingsType::BRAVE_SOLANA:
    case ContentSettingsType::BRAVE_CARDANO:
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
    case ContentSettingsType::BRAVE_LOCALHOST_ACCESS:
    case ContentSettingsType::BRAVE_OPEN_AI_CHAT:
      return true;
    default:
      return PermissionUtil_ChromiumImpl::IsPermission(type);
  }
}

PermissionType PermissionUtil::ContentSettingsTypeToPermissionType(
    ContentSettingsType permission) {
  switch (permission) {
    case ContentSettingsType::BRAVE_ADS:
      return PermissionType::BRAVE_ADS;
    case ContentSettingsType::BRAVE_TRACKERS:
      return PermissionType::BRAVE_TRACKERS;
    case ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES:
      return PermissionType::BRAVE_HTTP_UPGRADABLE_RESOURCES;
    case ContentSettingsType::BRAVE_FINGERPRINTING_V2:
      return PermissionType::BRAVE_FINGERPRINTING_V2;
    case ContentSettingsType::BRAVE_SHIELDS:
      return PermissionType::BRAVE_SHIELDS;
    case ContentSettingsType::BRAVE_REFERRERS:
      return PermissionType::BRAVE_REFERRERS;
    case ContentSettingsType::BRAVE_COOKIES:
      return PermissionType::BRAVE_COOKIES;
    case ContentSettingsType::BRAVE_SPEEDREADER:
      return PermissionType::BRAVE_SPEEDREADER;
    case ContentSettingsType::BRAVE_ETHEREUM:
      return PermissionType::BRAVE_ETHEREUM;
    case ContentSettingsType::BRAVE_SOLANA:
      return PermissionType::BRAVE_SOLANA;
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
      return PermissionType::BRAVE_GOOGLE_SIGN_IN;
    case ContentSettingsType::BRAVE_LOCALHOST_ACCESS:
      return PermissionType::BRAVE_LOCALHOST_ACCESS;
    case ContentSettingsType::BRAVE_OPEN_AI_CHAT:
      return PermissionType::BRAVE_OPEN_AI_CHAT;
    case ContentSettingsType::BRAVE_CARDANO:
      return PermissionType::BRAVE_CARDANO;
    default:
      return PermissionUtil_ChromiumImpl::ContentSettingsTypeToPermissionType(
          permission);
  }
}

GURL PermissionUtil::GetCanonicalOrigin(ContentSettingsType permission,
                                        const GURL& requesting_origin,
                                        const GURL& embedding_origin) {
  // Use requesting_origin which will have ethereum or solana address info.
  if (permission == ContentSettingsType::BRAVE_ETHEREUM ||
      permission == ContentSettingsType::BRAVE_SOLANA ||
      permission == ContentSettingsType::BRAVE_CARDANO) {
    return requesting_origin;
  }

  return PermissionUtil_ChromiumImpl::GetCanonicalOrigin(
      permission, requesting_origin, embedding_origin);
}

}  // namespace permissions
