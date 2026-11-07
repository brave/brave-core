// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/side_panel/ai_chat/ai_chat_full_page_link_observer.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

AIChatFullPageLinkObserver::AIChatFullPageLinkObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

AIChatFullPageLinkObserver::~AIChatFullPageLinkObserver() = default;

void AIChatFullPageLinkObserver::DidOpenRequestedURL(
    content::WebContents* new_contents,
    content::RenderFrameHost* source_render_frame_host,
    const GURL& url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    ui::PageTransition transition,
    bool started_from_context_menu,
    bool renderer_initiated) {
  // Only a link the page followed on the user's behalf takes the conversation's
  // place. A background tab (modifier-click), a new window and a popup all
  // leave the conversation visible where it is, so they must not move it, and
  // neither should a browser-driven open such as a context menu item.
  if (!renderer_initiated || started_from_context_menu ||
      disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB) {
    return;
  }

  // `new_contents` is not in the tab strip yet: this runs while it is still
  // being created, and the insertion positions and activates it against AI
  // Chat's tab. Detaching that tab now would pull it out from under the
  // insertion, and the in-flight window would be handed to the side panel's
  // delegate (which navigates the active tab instead of opening one). Let the
  // new tab land first and move once this open has finished.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&AIChatFullPageLinkObserver::MoveConversationToSidePanel,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AIChatFullPageLinkObserver::MoveConversationToSidePanel() {
  if (!web_contents()) {
    return;
  }

  // No-op unless the feature is enabled and this conversation is a full-page
  // tab in a window whose side panel is global.
  MaybeMoveFullPageChatToSidePanel(web_contents());
}

}  // namespace ai_chat
