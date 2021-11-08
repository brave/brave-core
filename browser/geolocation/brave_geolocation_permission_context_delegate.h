/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_CONTEXT_DELEGATE_H_
#define BRAVE_BROWSER_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_CONTEXT_DELEGATE_H_

#include "chrome/browser/geolocation/geolocation_permission_context_delegate.h"

class Profile;

class BraveGeolocationPermissionContextDelegate
    : public GeolocationPermissionContextDelegate {
 public:
  explicit BraveGeolocationPermissionContextDelegate(
      content::BrowserContext* browser_context);
  BraveGeolocationPermissionContextDelegate(
      const BraveGeolocationPermissionContextDelegate&) = delete;
  BraveGeolocationPermissionContextDelegate& operator=(
      const BraveGeolocationPermissionContextDelegate&) = delete;
  ~BraveGeolocationPermissionContextDelegate() override;

  bool DecidePermission(
      content::WebContents* web_contents,
      const permissions::PermissionRequestID& id,
      const GURL& requesting_origin,
      bool user_gesture,
      permissions::BrowserPermissionCallback* callback,
      permissions::GeolocationPermissionContext* context) override;

 private:
  Profile* profile_;
};

#endif  // BRAVE_BROWSER_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_CONTEXT_DELEGATE_H_
