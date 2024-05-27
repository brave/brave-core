/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"

#include <vector>

#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_add_bubble_view.h"
#include "brave/browser/ui/views/playlist/playlist_edit_bubble_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace playlist {

// static
PlaylistBubblesController*
PlaylistBubblesController::CreateOrGetFromWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);
  PlaylistBubblesController::CreateForWebContents(web_contents);
  return PlaylistBubblesController::FromWebContents(web_contents);
}

PlaylistBubblesController::~PlaylistBubblesController() = default;

void PlaylistBubblesController::ShowBubble(
    base::WeakPtr<PlaylistActionIconView> anchor_view,
    BubbleType bubble_type) {
  if (!anchor_view) {
    return;
  }

  CHECK(!bubble_);
  auto* tab_helper = PlaylistTabHelper::FromWebContents(&GetWebContents());
  CHECK(tab_helper);

  switch (bubble_type) {
    case BubbleType::kInfer:
      if (!tab_helper->saved_items().empty()) {
        bubble_ = new PlaylistEditBubbleView(anchor_view.get(),
                                             tab_helper->GetWeakPtr());
      } else if (!tab_helper->found_items().empty()) {
        bubble_ = new PlaylistAddBubbleView(anchor_view.get(),
                                            tab_helper->GetWeakPtr());
      } else {
        NOTREACHED_IN_MIGRATION()
            << "The action icon shouldn't be visible then.";
      }
      break;
    case BubbleType::kAdd:
      bubble_ = new PlaylistAddBubbleView(anchor_view.get(),
                                          tab_helper->GetWeakPtr());
      break;
    case BubbleType::kEdit:
      bubble_ = new PlaylistEditBubbleView(anchor_view.get(),
                                           tab_helper->GetWeakPtr());
      break;
  }

  CHECK(bubble_);
  views::BubbleDialogDelegateView::CreateBubble(
      base::WrapUnique(
          static_cast<views::BubbleDialogDelegateView*>(bubble_.get())))
      ->Show();
}

PlaylistBubbleView* PlaylistBubblesController::GetBubble() {
  return bubble_;
}

void PlaylistBubblesController::OnBubbleClosed() {
  bubble_ = nullptr;
}

base::WeakPtr<PlaylistBubblesController>
PlaylistBubblesController::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

PlaylistBubblesController::PlaylistBubblesController(
    content::WebContents* web_contents)
    : content::WebContentsUserData<PlaylistBubblesController>(*web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBubblesController);

}  // namespace playlist
