/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/ancestor_throttle.h"
#include "content/browser/renderer_host/navigation_request.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/public/browser/web_contents.h"

// Override WillProcessResponse to check permissions before CSP checks
#define WillProcessResponse WillProcessResponse_ChromiumImpl
#include <content/browser/renderer_host/ancestor_throttle.cc>
#undef WillProcessResponse

namespace content {

// Static member definition
AncestorThrottle::PermissionCallback* AncestorThrottle::permission_callback_ = nullptr;

// Static methods implementation
void AncestorThrottle::SetPermissionCallback(AncestorThrottle::PermissionCallback callback) {
  if (!permission_callback_) {
    permission_callback_ = new AncestorThrottle::PermissionCallback();
  }
  *permission_callback_ = std::move(callback);
}

bool AncestorThrottle::CheckPermissionForOrigin(content::BrowserContext* browser_context, const url::Origin& origin) {
  if (!permission_callback_ || permission_callback_->is_null()) {
    return false;
  }
  return permission_callback_->Run(browser_context, origin);
}

NavigationThrottle::ThrottleCheckResult AncestorThrottle::WillProcessResponse() {
  // Check if this navigation should bypass ancestor checks due to permissions
  NavigationRequest* request = NavigationRequest::From(navigation_handle());
  if (request && !request->IsInOutermostMainFrame()) {
    content::FrameTreeNode* frame_node = request->frame_tree_node();
    if (frame_node && frame_node->parent()) {
      // Only bypass CSP for the first level of inner frames (direct children of main frame)
      content::RenderFrameHost* parent_frame = frame_node->parent();
      if (parent_frame && parent_frame->IsInPrimaryMainFrame()) {
        url::Origin parent_origin = parent_frame->GetLastCommittedOrigin();
        content::BrowserContext* browser_context = navigation_handle()->GetWebContents()->GetBrowserContext();
        if (CheckPermissionForOrigin(browser_context, parent_origin)) {
          return NavigationThrottle::PROCEED;
        }
      }
    }
  }

  // Fall back to original implementation
  return WillProcessResponse_ChromiumImpl();
}

}  // namespace content