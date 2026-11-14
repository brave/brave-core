/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/permissions/permission_manager.cc>

namespace permissions {

content::PermissionResult
PermissionManager::GetPermissionResultForEmbeddedRequester(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    content::RenderFrameHost* render_frame_host,
    const url::Origin& requesting_origin) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const GURL embedding_origin =
      GetEmbeddingOrigin(render_frame_host, requesting_origin.GetURL());

  return GetPermissionStatusInternal(
      permission_descriptor,
      /*render_process_host=*/nullptr, render_frame_host,
      requesting_origin.GetURL(), embedding_origin,
      /*should_include_device_status=*/false);
}

}  // namespace permissions
