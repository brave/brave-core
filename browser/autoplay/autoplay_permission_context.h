/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AUTOPLAY_AUTOPLAY_PERMISSION_CONTEXT_H_
#define BRAVE_BROWSER_AUTOPLAY_AUTOPLAY_PERMISSION_CONTEXT_H_

#include "base/macros.h"
#include "components/permissions/permission_context_base.h"

class AutoplayPermissionContext : public permissions::PermissionContextBase {
 public:
  explicit AutoplayPermissionContext(content::BrowserContext* browser_context);
  ~AutoplayPermissionContext() override;

 private:
  // PermissionContextBase:
  void UpdateTabContext(const permissions::PermissionRequestID& id,
                        const GURL& requesting_frame,
                        bool allowed) override;
  void NotifyPermissionSet(const permissions::PermissionRequestID& id,
                           const GURL& requesting_origin,
                           const GURL& embedding_origin,
                           permissions::BrowserPermissionCallback callback,
                           bool persist,
                           ContentSetting content_setting) override;
  bool IsRestrictedToSecureOrigins() const override;

  DISALLOW_COPY_AND_ASSIGN(AutoplayPermissionContext);
};

#endif  // BRAVE_BROWSER_AUTOPLAY_AUTOPLAY_PERMISSION_CONTEXT_H_
