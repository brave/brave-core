/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_add_bubble.h"
#include "brave/browser/ui/views/playlist/playlist_confirm_bubble.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/widget/widget.h"

namespace {
views::BubbleDialogDelegateView* g_bubble = nullptr;
}  // namespace

void ShowBubble(std::unique_ptr<views::BubbleDialogDelegateView> bubble) {
  DCHECK(!g_bubble);

  g_bubble = bubble.release();

  auto* widget = views::BubbleDialogDelegateView::CreateBubble(g_bubble);
  widget->Show();
}

// static
void PlaylistActionBubbleView::ShowBubble(
    Browser* browser,
    PlaylistActionIconView* anchor,
    playlist::PlaylistTabHelper* playlist_tab_helper) {
  if (!playlist_tab_helper->saved_items().empty()) {
    ::ShowBubble(
        std::make_unique<ConfirmBubble>(browser, anchor, playlist_tab_helper));
  } else if (!playlist_tab_helper->found_items().empty()) {
    ::ShowBubble(std::make_unique<PlaylistAddBubble>(browser, anchor,
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

PlaylistActionBubbleView::PlaylistActionBubbleView(
    Browser* browser,
    PlaylistActionIconView* anchor,
    playlist::PlaylistTabHelper* playlist_tab_helper)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::Arrow::TOP_RIGHT),
      browser_(browser),
      playlist_tab_helper_(playlist_tab_helper),
      icon_view_(anchor) {}

PlaylistActionBubbleView::~PlaylistActionBubbleView() = default;

void PlaylistActionBubbleView::WindowClosing() {
  BubbleDialogDelegateView::WindowClosing();

  WindowClosingImpl();
}

void PlaylistActionBubbleView::CloseAndRun(base::OnceClosure callback) {
  SetCloseCallback(
      // WindowClosingImpl should be called first to clean up data before
      // showing up new bubble. This callback is called by itself, it's okay to
      // pass Unretained().
      base::BindOnce(&PlaylistActionBubbleView::WindowClosingImpl,
                     base::Unretained(this))
          .Then(std::move(callback)));

  GetWidget()->Close();
}

void PlaylistActionBubbleView::WindowClosingImpl() {
  // This method could be called multiple times during the closing process in
  // order to show up a subsequent action bubble. So we should check if
  // |g_bubble| is already filled up with a new bubble.
  if (g_bubble == this) {
    g_bubble = nullptr;
  }
}

BEGIN_METADATA(PlaylistActionBubbleView)
END_METADATA
