/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/brave_permission_manager.h"

#include <utility>

#include "base/auto_reset.h"
#include "components/permissions/permission_context_base.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/permission_request_description.h"
#include "url/origin.h"

namespace permissions {

BravePermissionManager::BravePermissionManager(
    content::BrowserContext* browser_context,
    PermissionContextMap permission_contexts)
    : PermissionManager(browser_context, std::move(permission_contexts)) {}

void BravePermissionManager::Shutdown() {
  PermissionManager::Shutdown();
  permission_contexts_.clear();
}

void BravePermissionManager::RequestPermissionsForOrigin(
    const std::vector<blink::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::AutoReset<GURL> auto_reset_requesting_origin(&forced_requesting_origin_,
                                                     requesting_origin);
  return RequestPermissionsFromCurrentDocument(
      render_frame_host,
      content::PermissionRequestDescription(
          content::PermissionDescriptorUtil::
              CreatePermissionDescriptorForPermissionTypes(permissions),
          user_gesture),
      std::move(callback));
}

blink::mojom::PermissionStatus
BravePermissionManager::GetPermissionStatusForOrigin(
    blink::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::AutoReset<GURL> auto_reset_requesting_origin(&forced_requesting_origin_,
                                                     requesting_origin);
  return GetPermissionStatusForCurrentDocument(
      permission, render_frame_host, /*should_include_device_status=*/false);
}

}  // namespace permissions
