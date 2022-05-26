/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_BRAVE_PERMISSION_MANAGER_H_
#define BRAVE_COMPONENTS_PERMISSIONS_BRAVE_PERMISSION_MANAGER_H_

#include <vector>

#include "components/permissions/permission_manager.h"

namespace permissions {

class BravePermissionManager : public PermissionManager {
 public:
  BravePermissionManager(content::BrowserContext* browser_context,
                         PermissionContextMap permission_contexts);
  ~BravePermissionManager() override = default;

  BravePermissionManager(const BravePermissionManager&) = delete;
  BravePermissionManager& operator=(const BravePermissionManager&) = delete;

  GURL GetCanonicalOrigin(ContentSettingsType permission,
                          const GURL& requesting_origin,
                          const GURL& embedding_origin) const override;

  void ResetPermissionViaContentSetting(ContentSettingsType type,
                                        const GURL& requesting_origin,
                                        const GURL& embedding_origin);

  void RequestPermissionsDeprecated(
      const std::vector<blink::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      base::OnceCallback<
          void(const std::vector<blink::mojom::PermissionStatus>&)> callback);

  blink::mojom::PermissionStatus GetPermissionStatusForFrame(
      blink::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin);

  PermissionResult GetPermissionStatusDeprecated(ContentSettingsType permission,
                                                 const GURL& requesting_origin,
                                                 const GURL& embedding_origin);
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_BRAVE_PERMISSION_MANAGER_H_
