// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_RESTRICTED_WEB_CONTENTS_DELEGATE_RESTRICTED_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_COMPONENTS_RESTRICTED_WEB_CONTENTS_DELEGATE_RESTRICTED_WEB_CONTENTS_DELEGATE_H_

#include <string>

#include "content/public/browser/web_contents_delegate.h"

namespace content {
class RenderFrameHost;
class SiteInstance;
class WebContents;
struct DropData;
}  // namespace content

// A WebContentsDelegate with safe defaults for background/hidden WebContents.
// Suppresses dialogs, downloads, popups, fullscreen, and drag-and-drop.
// Subclasses can override individual methods to relax restrictions as needed.
class RestrictedWebContentsDelegate : public content::WebContentsDelegate {
 public:
  RestrictedWebContentsDelegate();
  ~RestrictedWebContentsDelegate() override;

  // content::WebContentsDelegate:
  bool ShouldSuppressDialogs(content::WebContents* source) override;
  void CanDownload(const GURL& url,
                   const std::string& request_method,
                   base::OnceCallback<void(bool)> callback) override;
  bool IsWebContentsCreationOverridden(
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url) override;
  bool CanEnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame) override;
  bool CanDragEnter(content::WebContents* source,
                    const content::DropData& data,
                    blink::DragOperationsMask operations_allowed) override;
  void RequestKeyboardLock(content::WebContents* web_contents,
                           bool esc_key_locked) override;
};

#endif  // BRAVE_COMPONENTS_RESTRICTED_WEB_CONTENTS_DELEGATE_RESTRICTED_WEB_CONTENTS_DELEGATE_H_
