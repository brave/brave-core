/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_util.h"

#include "components/permissions/permission_uma_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#define PermissionUtil PermissionUtil_ChromiumImpl

#define PERMISSION_UTIL_PERMISSION_TYPE_TO_CONTENT_SETTINGS_TYPE \
  case PermissionType::BRAVE_ADS:                                \
    return ContentSettingsType::BRAVE_ADS;                       \
  case PermissionType::BRAVE_COSMETIC_FILTERING:                 \
    return ContentSettingsType::BRAVE_COSMETIC_FILTERING;        \
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
    return ContentSettingsType::BRAVE_OPEN_AI_CHAT;

#include "src/components/permissions/permission_util.cc"
#undef PermissionUtil
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
    default:
      return PermissionUtil_ChromiumImpl::GetPermissionString(content_type);
  }
}

// static
bool PermissionUtil::GetPermissionType(ContentSettingsType type,
                                       blink::PermissionType* out) {
  if (type == ContentSettingsType::BRAVE_ETHEREUM ||
      type == ContentSettingsType::BRAVE_SOLANA) {
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
    case ContentSettingsType::BRAVE_COSMETIC_FILTERING:
      return PermissionType::BRAVE_COSMETIC_FILTERING;
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
      permission == ContentSettingsType::BRAVE_SOLANA) {
    return requesting_origin;
  }

  return PermissionUtil_ChromiumImpl::GetCanonicalOrigin(
      permission, requesting_origin, embedding_origin);
}

}  // namespace permissions
