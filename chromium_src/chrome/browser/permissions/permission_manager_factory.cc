/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/permission_manager_factory.h"

#include <memory>

#include "brave/browser/autoplay/autoplay_permission_context.h"
#include "brave/browser/geolocation/brave_geolocation_permission_context.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace {

// Forward declaration.
permissions::PermissionManager::PermissionContextMap CreatePermissionContexts(
    Profile* profile);

permissions::PermissionManager::PermissionContextMap
BraveCreatePermissionContexts(Profile* profile) {
  permissions::PermissionManager::PermissionContextMap permission_contexts =
      CreatePermissionContexts(profile);
  permission_contexts[ContentSettingsType::AUTOPLAY] =
      std::make_unique<AutoplayPermissionContext>(profile);
  return permission_contexts;
}

}  // namespace

#define BuildServiceInstanceFor BuildServiceInstanceFor_ChromiumImpl
#define GeolocationPermissionContext BraveGeolocationPermissionContext
#include "../../../../../chrome/browser/permissions/permission_manager_factory.cc"
#undef GeolocationPermissionContext
#undef BuildServiceInstanceFor

KeyedService* PermissionManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return new permissions::PermissionManager(
      profile, BraveCreatePermissionContexts(profile));
}
