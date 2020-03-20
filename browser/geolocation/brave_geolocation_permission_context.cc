/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/geolocation/brave_geolocation_permission_context.h"

#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/prefs/pref_service.h"
#include "brave/common/tor/pref_names.h"

BraveGeolocationPermissionContext::BraveGeolocationPermissionContext(
    Profile* profile) : GeolocationPermissionContext(profile) {
}

BraveGeolocationPermissionContext::~BraveGeolocationPermissionContext() {
}

void BraveGeolocationPermissionContext::DecidePermission(
    content::WebContents* web_contents,
    const permissions::PermissionRequestID& id,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    bool user_gesture,
    BrowserPermissionCallback callback) {
  if (brave::IsTorProfile(profile())) {
    std::move(callback).Run(ContentSetting::CONTENT_SETTING_BLOCK);
    return;
  }

  return GeolocationPermissionContext::DecidePermission(
      web_contents, id, requesting_origin, embedding_origin, user_gesture,
      std::move(callback));
}
