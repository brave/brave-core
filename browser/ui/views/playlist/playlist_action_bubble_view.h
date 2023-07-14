/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

class Browser;

namespace playlist {
class PlaylistTabHelper;
}  // namespace playlist

#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class PlaylistActionBubbleView : public views::BubbleDialogDelegateView {
 public:
  METADATA_HEADER(PlaylistActionBubbleView);

  static void ShowBubble(Browser* browser,
                         views::View* anchor,
                         playlist::PlaylistTabHelper* playlist_tab_helper);
  static bool IsShowingBubble();
  static void CloseBubble();
  static PlaylistActionBubbleView* GetBubble();

  ~PlaylistActionBubbleView() override;

  // views::BubbleDialogDelegateView:
  void WindowClosing() override;

 protected:
  PlaylistActionBubbleView(Browser* browser,
                           views::View* anchor,
                           playlist::PlaylistTabHelper* playlist_tab_helper);

  raw_ptr<Browser> browser_;
  raw_ptr<playlist::PlaylistTabHelper> playlist_tab_helper_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
