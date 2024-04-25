/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;

namespace playlist {
class PlaylistTabHelper;

class PlaylistActionBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(PlaylistActionBubbleView, views::BubbleDialogDelegateView)
 public:
  ~PlaylistActionBubbleView() override;

 protected:
  PlaylistActionBubbleView(
      Browser* browser,
      View* anchor_view,
      base::WeakPtr<PlaylistTabHelper> playlist_tab_helper);

  // BubbleDialogDelegate:
  void OnWidgetDestroyed(views::Widget* widget) override;

  base::WeakPtr<PlaylistBubblesController> controller_;
  raw_ptr<Browser> browser_ = nullptr;
  base::WeakPtr<PlaylistTabHelper> tab_helper_;
  int next_bubble_ = 0;
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
