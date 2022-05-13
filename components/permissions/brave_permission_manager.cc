/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/brave_permission_manager.h"

#include <utility>

#include "components/permissions/permission_context_base.h"
#include "content/public/browser/browser_thread.h"
#include "url/origin.h"

namespace permissions {

BravePermissionManager::BravePermissionManager(
    content::BrowserContext* browser_context,
    PermissionContextMap permission_contexts)
    : PermissionManager(browser_context, std::move(permission_contexts)) {}

GURL BravePermissionManager::GetCanonicalOrigin(
    ContentSettingsType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  // Use requesting_origin which will have ethereum or solana address info.
  if (permission == ContentSettingsType::BRAVE_ETHEREUM ||
      permission == ContentSettingsType::BRAVE_SOLANA)
    return requesting_origin;

  return PermissionManager::GetCanonicalOrigin(permission, requesting_origin,
                                               embedding_origin);
}

void BravePermissionManager::ResetPermissionViaContentSetting(
    ContentSettingsType type,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PermissionContextBase* context = GetPermissionContext(type);
  if (!context)
    return;
  context->ResetPermission(
      GetCanonicalOrigin(type, requesting_origin, embedding_origin),
      url::Origin::Create(embedding_origin).GetURL());
}

void BravePermissionManager::RequestPermissionsDeprecated(
    const std::vector<blink::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  RequestPermissions(permissions, render_frame_host, requesting_origin,
                     user_gesture, std::move(callback));
}

blink::mojom::PermissionStatus
BravePermissionManager::GetPermissionStatusForFrame(
    blink::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ContentSettingsType type =
      PermissionUtil::PermissionTypeToContentSetting(permission);

  const GURL embedding_origin =
      GetEmbeddingOriginInternal(render_frame_host, requesting_origin);

  PermissionResult result = GetPermissionStatusHelper(
      type,
      /*render_process_host=*/nullptr, render_frame_host, requesting_origin,
      embedding_origin);

  return ContentSettingToPermissionStatusInternal(result.content_setting);
}

PermissionResult BravePermissionManager::GetPermissionStatusDeprecated(
    ContentSettingsType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return PermissionManager::GetPermissionStatusDeprecated(
      permission, requesting_origin, embedding_origin);
}

}  // namespace permissions
