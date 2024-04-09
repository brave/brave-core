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
namespace {
views::BubbleDialogDelegateView* g_bubble = nullptr;
}  // namespace

// static
void PlaylistActionBubbleView::ShowBubble(
    Browser* browser,
    PlaylistActionIconView* anchor,
    PlaylistTabHelper* playlist_tab_helper) {
  if (!playlist_tab_helper->saved_items().empty()) {
    ShowBubble(std::make_unique<PlaylistConfirmBubble>(browser, anchor,
                                                       playlist_tab_helper));
  } else if (!playlist_tab_helper->found_items().empty()) {
    ShowBubble(std::make_unique<PlaylistAddBubble>(browser, anchor,
                                                   playlist_tab_helper));
  }
}

// static
bool PlaylistActionBubbleView::IsShowingBubble() {
  return g_bubble && g_bubble->GetWidget() &&
         !g_bubble->GetWidget()->IsClosed();
}

// static
void PlaylistActionBubbleView::CloseBubble() {
  g_bubble->GetWidget()->Close();
}

// static
views::BubbleDialogDelegateView* PlaylistActionBubbleView::GetBubble() {
  return g_bubble;
}

// static
void PlaylistActionBubbleView::ShowBubble(
    std::unique_ptr<views::BubbleDialogDelegateView> bubble) {
  if (g_bubble) {
    g_bubble->GetWidget()->Close();
  }

  g_bubble = bubble.release();

  auto* widget = views::BubbleDialogDelegateView::CreateBubble(g_bubble);
  widget->Show();
}

PlaylistActionBubbleView::PlaylistActionBubbleView(
    Browser* browser,
    PlaylistActionIconView* anchor,
    PlaylistTabHelper* playlist_tab_helper)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::Arrow::TOP_RIGHT),
      browser_(browser),
      playlist_tab_helper_(playlist_tab_helper),
      icon_view_(anchor) {}

PlaylistActionBubbleView::~PlaylistActionBubbleView() = default;

void PlaylistActionBubbleView::WindowClosing() {
  BubbleDialogDelegateView::WindowClosing();

  g_bubble = nullptr;
}

BEGIN_METADATA(PlaylistActionBubbleView)
END_METADATA
}  // namespace playlist
