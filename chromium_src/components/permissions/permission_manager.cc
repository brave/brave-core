/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/permissions/permission_manager.cc"

namespace permissions {

GURL PermissionManager::GetEmbeddingOriginInternal(
    content::RenderFrameHost* const render_frame_host,
    const GURL& requesting_origin) {
  return GetEmbeddingOrigin(render_frame_host, requesting_origin);
}

PermissionStatus PermissionManager::ContentSettingToPermissionStatusInternal(
    ContentSetting setting) {
  return ContentSettingToPermissionStatus(setting);
}

}  // namespace permissions
