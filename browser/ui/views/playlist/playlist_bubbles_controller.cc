/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"

#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_add_bubble.h"
#include "brave/browser/ui/views/playlist/playlist_confirm_bubble.h"
#include "chrome/browser/ui/browser_finder.h"

namespace playlist {

// static
PlaylistBubblesController*
PlaylistBubblesController::CreateOrGetFromWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);
  PlaylistBubblesController::CreateForWebContents(web_contents);
  return PlaylistBubblesController::FromWebContents(web_contents);
}

PlaylistBubblesController::~PlaylistBubblesController() {
  if (bubble_) {
    bubble_->Hide();
  }
}

void PlaylistBubblesController::ShowBubble(base::WeakPtr<PlaylistActionIconView> anchor_view) {
  DVLOG(2) << __FUNCTION__;

  auto* tab_helper = PlaylistTabHelper::FromWebContents(&GetWebContents());
  CHECK(tab_helper);

  Browser* browser = chrome::FindBrowserWithTab(&GetWebContents());
  playlist::PlaylistActionBubbleView::ShowBubble(browser, anchor_view,
                                                 tab_helper->GetWeakPtr());
}

PlaylistActionBubbleView* PlaylistBubblesController::GetBubble() const {
  return bubble_;
}

void PlaylistBubblesController::OnBubbleClosed() {
  bubble_ = nullptr;
}

PlaylistBubblesController::PlaylistBubblesController(
    content::WebContents* web_contents)
    : content::WebContentsUserData<PlaylistBubblesController>(
          *web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBubblesController);

}  // namespace playlist
