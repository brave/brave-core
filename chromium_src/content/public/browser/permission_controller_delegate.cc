/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/public/browser/permission_controller_delegate.cc"

namespace content {

blink::mojom::PermissionStatus
PermissionControllerDelegate::GetPermissionStatusForOrigin(
    blink::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  return blink::mojom::PermissionStatus::DENIED;
}

}  // namespace content
