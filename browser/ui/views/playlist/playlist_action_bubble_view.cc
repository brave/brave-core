/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

namespace {

PlaylistActionBubbleView* g_bubble = nullptr;

}  // namespace

// static
void PlaylistActionBubbleView::ShowBubble(views::View* anchor,
                                          content::WebContents* contents) {
  DCHECK(!g_bubble);

  g_bubble = new PlaylistActionBubbleView(anchor, contents);
  auto* widget = views::BubbleDialogDelegateView::CreateBubble(g_bubble);
  widget->Show();
}

// static
bool PlaylistActionBubbleView::IsShowingBubble() {
  return g_bubble;
}

// static
void PlaylistActionBubbleView::CloseBubble() {
  g_bubble->GetWidget()->Close();
}

// static
views::BubbleDialogDelegateView* PlaylistActionBubbleView::GetBubble() {
  return g_bubble;
}

PlaylistActionBubbleView::PlaylistActionBubbleView(
    views::View* anchor,
    content::WebContents* contents)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::Arrow::TOP_RIGHT) {
  SetPreferredSize(gfx::Size(100, 100));
  SetButtons(ui::DIALOG_BUTTON_NONE);
}

PlaylistActionBubbleView::~PlaylistActionBubbleView() = default;

void PlaylistActionBubbleView::WindowClosing() {
  BubbleDialogDelegateView::WindowClosing();

  DCHECK_EQ(g_bubble, this);
  g_bubble = nullptr;
}

BEGIN_METADATA(PlaylistActionBubbleView, views::BubbleDialogDelegateView)
END_METADATA
