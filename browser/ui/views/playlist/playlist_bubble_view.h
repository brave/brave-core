/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLE_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace views {
class View;
class Widget;
}  // namespace views

namespace playlist {
class PlaylistTabHelper;

class PlaylistBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(PlaylistBubbleView, views::BubbleDialogDelegateView)

 protected:
  PlaylistBubbleView(views::View* anchor_view,
                     base::WeakPtr<PlaylistTabHelper> tab_helper);

  ~PlaylistBubbleView() override = 0;

  // BubbleDialogDelegate:
  void OnWidgetDestroyed(views::Widget* widget) override;

  base::WeakPtr<PlaylistTabHelper> tab_helper_;
  PlaylistBubblesController::BubbleType next_bubble_ =
      PlaylistBubblesController::BubbleType::kInfer;

 private:
  base::WeakPtr<PlaylistBubblesController> controller_;
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_BUBBLE_VIEW_H_
