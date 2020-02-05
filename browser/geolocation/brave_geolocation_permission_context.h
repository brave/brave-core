/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_CONTEXT_H_
#define BRAVE_BROWSER_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_CONTEXT_H_

#include "chrome/browser/geolocation/geolocation_permission_context.h"

class BraveGeolocationPermissionContext : public GeolocationPermissionContext {
 public:
  explicit BraveGeolocationPermissionContext(Profile* profile);
  ~BraveGeolocationPermissionContext() override;

  void DecidePermission(content::WebContents* web_contents,
                        const permissions::PermissionRequestID& id,
                        const GURL& requesting_origin,
                        const GURL& embedding_origin,
                        bool user_gesture,
                        BrowserPermissionCallback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveGeolocationPermissionContext);
};

#endif  // BRAVE_BROWSER_GEOLOCATION_BRAVE_GEOLOCATION_PERMISSION_CONTEXT_H_
