/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Hooks called from the chromium_src override of RenderViewContextMenu. They
// are declared there, so no header is needed.

#include <utility>

#include "base/memory/scoped_refptr.h"
#include "brave/browser/ui/peek/peek_utils.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "content/public/browser/initiator_navigation_state.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace peek {

bool CanOpenLinkInPeek(BrowserWindowInterface* browser,
                       content::WebContents* source,
                       const GURL& url,
                       const url::Origin& frame_origin) {
  if (!IsPeekableURL(url) || !IsPeekableInitiator(frame_origin)) {
    return false;
  }
  auto* controller = PeekController::From(browser);
  return controller && controller->CanOpenFrom(source);
}

void OpenLinkInPeek(BrowserWindowInterface* browser,
                    content::WebContents* source,
                    content::RenderFrameHost* initiator_frame,
                    const url::Origin& frame_origin,
                    content::OpenURLParams params) {
  auto* controller = PeekController::From(browser);
  if (!controller || !source) {
    return;
  }

  // The menu can outlive the document it was opened on.
  if (!initiator_frame || !initiator_frame->IsActive() ||
      initiator_frame->GetLastCommittedOrigin() != frame_origin) {
    return;
  }
  scoped_refptr<content::InitiatorNavigationState> initiator_state =
      initiator_frame->GetCurrentInitiatorNavigationState();
  if (!initiator_state) {
    return;
  }

  // Load the link as if it was clicked in `initiator_frame`, with the frame's
  // policies and not only its origin. Without them the load skips checks that
  // apply to the initiator, such as Local Network Access.
  params.is_renderer_initiated = true;
  params.user_gesture = true;
  params.initiator_navigation_state = std::move(initiator_state);
  params.initiator_origin = initiator_frame->GetLastCommittedOrigin();
  params.initiator_frame_token = initiator_frame->GetFrameToken();
  params.initiator_process_id =
      initiator_frame->GetProcess()->GetID().GetUnsafeValue();
  params.source_site_instance = initiator_frame->GetSiteInstance();
  controller->Open(*source, params);
}

}  // namespace peek
