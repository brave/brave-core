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
PlaylistActionBubbleView* g_bubble = nullptr;
}  // namespace

// static
void PlaylistActionBubbleView::ShowBubble(
    Browser* browser,
    base::WeakPtr<PlaylistActionIconView> action_icon_view,
    base::WeakPtr<PlaylistTabHelper> tab_helper) {
  CHECK(tab_helper);
  if (!tab_helper->saved_items().empty()) {
    ShowBubble(std::make_unique<PlaylistConfirmBubble>(
        browser, std::move(action_icon_view), std::move(tab_helper)));
  } else if (!tab_helper->found_items().empty()) {
    ShowBubble(std::make_unique<PlaylistAddBubble>(
        browser, std::move(action_icon_view), std::move(tab_helper)));
  }
}

// static
bool PlaylistActionBubbleView::IsShowingBubble() {
  return g_bubble && g_bubble->GetWidget() &&
         !g_bubble->GetWidget()->IsClosed();
}

// static
void PlaylistActionBubbleView::MaybeCloseBubble() {
  if (IsShowingBubble()) {
    g_bubble->GetWidget()->Close();
  }
}

// static
PlaylistActionBubbleView* PlaylistActionBubbleView::GetBubble() {
  return g_bubble;
}

// static
void PlaylistActionBubbleView::ShowBubble(
    std::unique_ptr<PlaylistActionBubbleView> bubble) {
  MaybeCloseBubble();

  g_bubble = bubble.release();

  auto* widget = views::BubbleDialogDelegateView::CreateBubble(g_bubble);
  widget->Show();
}

PlaylistActionBubbleView::PlaylistActionBubbleView(
    Browser* browser,
    base::WeakPtr<PlaylistActionIconView> action_icon_view,
    base::WeakPtr<PlaylistTabHelper> tab_helper)
    : BubbleDialogDelegateView(action_icon_view.get(),
                               views::BubbleBorder::Arrow::TOP_RIGHT),
      browser_(browser),
      action_icon_view_(std::move(action_icon_view)),
      tab_helper_(std::move(tab_helper)) {
  CHECK(browser_ && action_icon_view_ && tab_helper_);
}

PlaylistActionBubbleView::~PlaylistActionBubbleView() = default;

void PlaylistActionBubbleView::WindowClosing() {
  BubbleDialogDelegateView::WindowClosing();

  if (g_bubble == this) {
    g_bubble = nullptr;
  }
}

BEGIN_METADATA(PlaylistActionBubbleView)
END_METADATA
}  // namespace playlist
