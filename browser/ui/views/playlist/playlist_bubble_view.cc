/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_bubble_view.h"

#include <utility>

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/view_utils.h"

namespace playlist {

PlaylistBubbleView::PlaylistBubbleView(
    views::View* anchor_view,
    base::WeakPtr<PlaylistTabHelper> tab_helper)
    : BubbleDialogDelegateView(anchor_view,
                               views::BubbleBorder::Arrow::TOP_RIGHT),
      tab_helper_(std::move(tab_helper)) {
  CHECK(anchor_view);
  CHECK(tab_helper_);

  auto* controller = PlaylistBubblesController::FromWebContents(
      &tab_helper_->GetWebContents());
  CHECK(controller);
  controller_ = controller->AsWeakPtr();
}

PlaylistBubbleView::~PlaylistBubbleView() = default;

void PlaylistBubbleView::OnWidgetDestroyed(views::Widget*) {
  if (!controller_) {
    return;
  }

  controller_->OnBubbleClosed();

  if (next_bubble_ == PlaylistBubblesController::BubbleType::kInfer) {
    return;
  }

  auto* anchor_view = GetAnchorView();
  if (!anchor_view) {
    return;
  }

  auto* action_icon_view =
      views::AsViewClass<PlaylistActionIconView>(anchor_view);
  CHECK(action_icon_view);
  // Post as task, otherwise
  // "|anchor_view| has already anchored a focusable widget."
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistBubblesController::ShowBubble, controller_,
                     action_icon_view->AsWeakPtr(), next_bubble_));
}

BEGIN_METADATA(PlaylistBubbleView)
END_METADATA
}  // namespace playlist
