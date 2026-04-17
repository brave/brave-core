// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/restricted_web_contents_delegate/restricted_web_contents_delegate.h"

#include <utility>

#include "content/public/browser/web_contents.h"

RestrictedWebContentsDelegate::RestrictedWebContentsDelegate() = default;
RestrictedWebContentsDelegate::~RestrictedWebContentsDelegate() = default;

bool RestrictedWebContentsDelegate::ShouldSuppressDialogs(
    content::WebContents* source) {
  return true;
}

void RestrictedWebContentsDelegate::CanDownload(
    const GURL& url,
    const std::string& request_method,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(false);
}

bool RestrictedWebContentsDelegate::IsWebContentsCreationOverridden(
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url) {
  return true;
}

bool RestrictedWebContentsDelegate::CanEnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame) {
  return false;
}

bool RestrictedWebContentsDelegate::CanDragEnter(
    content::WebContents* source,
    const content::DropData& data,
    blink::DragOperationsMask operations_allowed) {
  return false;
}

void RestrictedWebContentsDelegate::RequestKeyboardLock(
    content::WebContents* web_contents,
    bool esc_key_locked) {
  web_contents->GotResponseToKeyboardLockRequest(false);
}
