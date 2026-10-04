// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_FULL_PAGE_LINK_OBSERVER_H_
#define BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_FULL_PAGE_LINK_OBSERVER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

class GURL;

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace ai_chat {

// Moves a full-page AI Chat conversation into the side panel when a link in it
// opens a new foreground tab, so the linked page takes the conversation's place
// rather than covering it.
//
// Conversation links are plain `<a target="_blank">` anchors, so the browser
// opens them through its normal new-window path without any AI Chat handler
// being involved. `DidOpenRequestedURL` is the browser-side signal that one was
// followed. Links AI Chat opens itself (`AIChatUIHandler::OpenURL` and friends)
// are browser-initiated and never reach here, so they keep moving the
// conversation from their own call sites.
//
// This observes AI Chat's `WebContents` wherever it is hosted; the move is a
// no-op while the conversation is in the side panel already (and on platforms
// with no side panel), so the observer does not need to track which surface
// hosts it.
class AIChatFullPageLinkObserver : public content::WebContentsObserver {
 public:
  explicit AIChatFullPageLinkObserver(content::WebContents* web_contents);
  AIChatFullPageLinkObserver(const AIChatFullPageLinkObserver&) = delete;
  AIChatFullPageLinkObserver& operator=(const AIChatFullPageLinkObserver&) =
      delete;
  ~AIChatFullPageLinkObserver() override;

 private:
  // content::WebContentsObserver:
  void DidOpenRequestedURL(content::WebContents* new_contents,
                           content::RenderFrameHost* source_render_frame_host,
                           const GURL& url,
                           const content::Referrer& referrer,
                           WindowOpenDisposition disposition,
                           ui::PageTransition transition,
                           bool started_from_context_menu,
                           bool renderer_initiated) override;

  void MoveConversationToSidePanel();

  base::WeakPtrFactory<AIChatFullPageLinkObserver> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_FULL_PAGE_LINK_OBSERVER_H_
