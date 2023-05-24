/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

namespace content {
class WebContents;
}  // namespace content

#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class PlaylistActionBubbleView : public views::BubbleDialogDelegateView {
 public:
  METADATA_HEADER(PlaylistActionBubbleView);

  static void ShowBubble(views::View* anchor, content::WebContents* contents);
  static bool IsShowingBubble();
  static void CloseBubble();
  static views::BubbleDialogDelegateView* GetBubble();

  ~PlaylistActionBubbleView() override;

  // views::BubbleDialogDelegateView:
  void WindowClosing() override;

 private:
  PlaylistActionBubbleView(views::View* anchor, content::WebContents* contents);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
