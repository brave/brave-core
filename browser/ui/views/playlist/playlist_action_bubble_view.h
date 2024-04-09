/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;
class PlaylistActionIconView;

namespace playlist {
class PlaylistTabHelper;

class PlaylistActionBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(PlaylistActionBubbleView, views::BubbleDialogDelegateView)
 public:
  static void ShowBubble(Browser* browser,
                         PlaylistActionIconView* anchor,
                         PlaylistTabHelper* playlist_tab_helper);
  static bool IsShowingBubble();
  static void CloseBubble();
  static views::BubbleDialogDelegateView* GetBubble();

 protected:
  static void ShowBubble(
      std::unique_ptr<views::BubbleDialogDelegateView> bubble);

  PlaylistActionBubbleView(Browser* browser,
                           PlaylistActionIconView* anchor,
                           PlaylistTabHelper* playlist_tab_helper);

  ~PlaylistActionBubbleView() override;

  // views::BubbleDialogDelegateView:
  void WindowClosing() override;

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<PlaylistTabHelper> playlist_tab_helper_ = nullptr;

  // Our anchor.
  raw_ptr<PlaylistActionIconView> icon_view_ = nullptr;
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
