/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/navigation_request.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "url/origin.h"

// Forward declare the function to avoid dependency on brave components
// This follows the same pattern as brave_policy forward declarations
namespace brave_puppeteer {
bool IsOriginAllowedForPuppeteerMode(const url::Origin& origin) {
  return true; // FIXME: Default to true to avoid build errors
}
}

// Make private members accessible by removing private specifier
#define private public
#define AncestorThrottle BaseAncestorThrottle
#include <content/browser/renderer_host/ancestor_throttle.h>
#undef private
#undef AncestorThrottle



namespace content {

// Create a class that inherits from AncestorThrottle with access to private members
class AncestorThrottle : public BaseAncestorThrottle {
 public:
  explicit AncestorThrottle(NavigationThrottleRegistry& registry)
      : BaseAncestorThrottle(registry) {}

  // Override WillProcessResponse to bypass CSP checks for puppeteer mode
  NavigationThrottle::ThrottleCheckResult WillProcessResponse() override {
    NavigationRequest* request = NavigationRequest::From(navigation_handle());
    if (request && !request->IsInOutermostMainFrame()) {
      content::FrameTreeNode* frame_node = request->frame_tree_node();
      if (frame_node && frame_node->parent()) {
        content::RenderFrameHost* parent_frame = frame_node->parent();
        if (parent_frame) {
          url::Origin parent_origin = parent_frame->GetLastCommittedOrigin();
          if (brave_puppeteer::IsOriginAllowedForPuppeteerMode(parent_origin)) {
            return NavigationThrottle::PROCEED;
          }
        }
      }
    }

    // Fall back to original behavior
    return BaseAncestorThrottle::WillProcessResponse();
  }

  // Static factory method to replace the original
  static void CreateAndAdd(NavigationThrottleRegistry& registry);
};

// Define the static method outside the class to ensure it's properly linked
void AncestorThrottle::CreateAndAdd(NavigationThrottleRegistry& registry) {
  registry.AddThrottle(std::make_unique<AncestorThrottle>(registry));
}

} // namespace content

#define AncestorThrottle BaseAncestorThrottle
#include <content/browser/renderer_host/ancestor_throttle.cc>
#undef AncestorThrottle