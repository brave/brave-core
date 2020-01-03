/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autoplay/autoplay_permission_context.h"
#include "brave/browser/geolocation/brave_geolocation_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/permission_type.h"

using content::PermissionType;

namespace {

ContentSettingsType PermissionTypeToContentSettingSafe_ChromiumImpl(
    PermissionType permission);

ContentSettingsType PermissionTypeToContentSettingSafe(
    PermissionType permission) {
  if (permission == PermissionType::AUTOPLAY)
    return ContentSettingsType::AUTOPLAY;
  return PermissionTypeToContentSettingSafe_ChromiumImpl(permission);
}

}  // namespace

#define GeolocationPermissionContext BraveGeolocationPermissionContext
#define PermissionManagerFactory BravePermissionManagerFactory
#include "../../../../../chrome/browser/permissions/permission_manager.cc"
#undef GeolocationPermissionContext
