/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/public/browser/permission_controller_delegate.cc>

namespace content {

PermissionResult
PermissionControllerDelegate::GetPermissionResultForEmbeddedRequester(
    const blink::mojom::PermissionDescriptorPtr& permission_descriptor,
    RenderFrameHost* render_frame_host,
    const url::Origin& requesting_origin) {
  return PermissionResult(PermissionStatus::DENIED);
}

}  // namespace content
