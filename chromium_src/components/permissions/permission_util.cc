/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_util.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

#define PermissionUtil PermissionUtil_ChromiumImpl

#define NUM                                                      \
  BRAVE_ADS:                                                     \
  return ContentSettingsType::BRAVE_ADS;                         \
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
  case PermissionType::NUM

#include "src/components/permissions/permission_util.cc"
#undef NUM
#undef PermissionUtil

namespace permissions {

// static
std::string PermissionUtil::GetPermissionString(
    ContentSettingsType content_type) {
  switch (content_type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return "BraveEthereum";
    case ContentSettingsType::BRAVE_SOLANA:
      return "BraveSolana";
    default:
      return PermissionUtil_ChromiumImpl::GetPermissionString(content_type);
  }
}

// static
bool PermissionUtil::GetPermissionType(ContentSettingsType type,
                                       blink::PermissionType* out) {
  if (type == ContentSettingsType::BRAVE_ETHEREUM ||
      type == ContentSettingsType::BRAVE_SOLANA) {
    *out = PermissionType::WINDOW_PLACEMENT;
    return true;
  }

  return PermissionUtil_ChromiumImpl::GetPermissionType(type, out);
}

// static
bool PermissionUtil::IsPermission(ContentSettingsType type) {
  switch (type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
    case ContentSettingsType::BRAVE_SOLANA:
      return true;
    default:
      return PermissionUtil_ChromiumImpl::IsPermission(type);
  }
}

}  // namespace permissions
