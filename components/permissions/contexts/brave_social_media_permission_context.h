/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_SOCIAL_MEDIA_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_SOCIAL_MEDIA_PERMISSION_CONTEXT_H_

#include "components/permissions/permission_context_base.h"

namespace permissions {

class BraveSocialMediaPermissionContext : public PermissionContextBase {
 public:
  // using PermissionContextBase::RequestPermission;
  explicit BraveSocialMediaPermissionContext(
      content::BrowserContext* browser_context);
  ~BraveSocialMediaPermissionContext() override;

  BraveSocialMediaPermissionContext(const BraveSocialMediaPermissionContext&) =
      delete;
  BraveSocialMediaPermissionContext& operator=(
      const BraveSocialMediaPermissionContext&) = delete;

 private:
  bool IsRestrictedToSecureOrigins() const override;
  void UpdateTabContext(const PermissionRequestID& id,
                        const GURL& requesting_frame,
                        bool allowed) override;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_SOCIAL_MEDIA_PERMISSION_CONTEXT_H_
