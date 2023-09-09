/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;
class PlaylistActionIconView;

namespace playlist {
class PlaylistTabHelper;
}  // namespace playlist

class PlaylistActionBubbleView : public views::BubbleDialogDelegateView {
 public:
  METADATA_HEADER(PlaylistActionBubbleView);

  static void ShowBubble(Browser* browser,
                         PlaylistActionIconView* anchor,
                         playlist::PlaylistTabHelper* playlist_tab_helper);
  static bool IsShowingBubble();
  static void CloseBubble();
  static PlaylistActionBubbleView* GetBubble();

  ~PlaylistActionBubbleView() override;

  void WindowClosingImpl();

  // views::BubbleDialogDelegateView:
  void WindowClosing() override;

 protected:
  PlaylistActionBubbleView(Browser* browser,
                           PlaylistActionIconView* anchor,
                           playlist::PlaylistTabHelper* playlist_tab_helper);

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<playlist::PlaylistTabHelper> playlist_tab_helper_ = nullptr;

  // Our anchor.
  raw_ptr<PlaylistActionIconView> icon_view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
