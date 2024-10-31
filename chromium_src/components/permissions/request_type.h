/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_REQUEST_TYPE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_REQUEST_TYPE_H_

#define kStorageAccess                                                       \
  kStorageAccess, kWidevine, kBraveEthereum, kBraveSolana, kBraveOpenAIChat, \
      kBraveGoogleSignInPermission, kBraveLocalhostAccessPermission,         \
      kBraveMinValue = kWidevine,                                            \
      kBraveMaxValue = kBraveLocalhostAccessPermission

#define ContentSettingsTypeToRequestType \
  ContentSettingsTypeToRequestType_ChromiumImpl

#define RequestTypeToContentSettingsType \
  RequestTypeToContentSettingsType_ChromiumImpl

#define IsRequestablePermissionType IsRequestablePermissionType_ChromiumImpl

#include <optional>

#include "src/components/permissions/request_type.h"  // IWYU pragma: export

#undef kStorageAccess
#undef ContentSettingsTypeToRequestType
#undef RequestTypeToContentSettingsType
#undef IsRequestablePermissionType

namespace permissions {

RequestType ContentSettingsTypeToRequestType(
    ContentSettingsType content_settings_type);

std::optional<ContentSettingsType> RequestTypeToContentSettingsType(
    RequestType request_type);

bool IsRequestablePermissionType(ContentSettingsType content_settings_type);

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_REQUEST_TYPE_H_
