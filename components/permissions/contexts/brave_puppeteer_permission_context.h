// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_PUPPETEER_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_PUPPETEER_PERMISSION_CONTEXT_H_

#include "components/permissions/permission_context_base.h"
#include "url/origin.h"

class BravePuppeteerPermissionContext : public permissions::PermissionContextBase {
 public:
  explicit BravePuppeteerPermissionContext(content::BrowserContext* browser_context);
  ~BravePuppeteerPermissionContext() override;

  BravePuppeteerPermissionContext(const BravePuppeteerPermissionContext&) = delete;
  BravePuppeteerPermissionContext& operator=(const BravePuppeteerPermissionContext&) = delete;

  // Returns true if the origin is allowed to use puppeteer mode
  static bool IsOriginAllowedForPuppeteerMode(content::BrowserContext* browser_context, const url::Origin& origin);

 private:
  // PermissionContextBase implementation
  bool IsRestrictedToSecureOrigins() const override;
  PermissionSetting GetPermissionStatusInternal(
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      const GURL& embedding_origin) const override;
};

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_PUPPETEER_PERMISSION_CONTEXT_H_