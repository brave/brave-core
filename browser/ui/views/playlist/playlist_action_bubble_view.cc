/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"

#include <vector>

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_add_bubble.h"
#include "brave/browser/ui/views/playlist/playlist_confirm_bubble.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/widget/widget.h"

namespace playlist {

PlaylistActionBubbleView::PlaylistActionBubbleView(
    Browser* browser,
    View* anchor_view,
    base::WeakPtr<PlaylistTabHelper> tab_helper)
    : BubbleDialogDelegateView(anchor_view,
                               views::BubbleBorder::Arrow::TOP_RIGHT),
      controller_(PlaylistBubblesController::CreateOrGetFromWebContents(
                      &tab_helper->GetWebContents())
                      ->AsWeakPtr()),
      browser_(browser),
      tab_helper_(std::move(tab_helper)) {
  CHECK(browser_ && anchor_view && tab_helper_);
}

PlaylistActionBubbleView::~PlaylistActionBubbleView() = default;

void PlaylistActionBubbleView::OnWidgetDestroyed(views::Widget* widget) {
  DVLOG(2) << __FUNCTION__;
  controller_->OnBubbleClosed();
  if (next_bubble_) {
    // otherwise: "|anchor_view| has already anchored a focusable widget."
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&PlaylistBubblesController::ShowBubble,
                                  controller_, GetAnchorView(), next_bubble_));
  }
}

BEGIN_METADATA(PlaylistActionBubbleView)
END_METADATA
}  // namespace playlist
