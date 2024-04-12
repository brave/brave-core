/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
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
                         base::WeakPtr<PlaylistActionIconView> anchor,
                         base::WeakPtr<PlaylistTabHelper> playlist_tab_helper);
  static bool IsShowingBubble();
  static void MaybeCloseBubble();
  static PlaylistActionBubbleView* GetBubble();

  ~PlaylistActionBubbleView() override;

 protected:
  static void ShowBubble(std::unique_ptr<PlaylistActionBubbleView> bubble);

  PlaylistActionBubbleView(
      Browser* browser,
      base::WeakPtr<PlaylistActionIconView> anchor,
      base::WeakPtr<PlaylistTabHelper> playlist_tab_helper);

  // views::WidgetDelegate:
  void WindowClosing() override;

  raw_ptr<Browser> browser_ = nullptr;
  base::WeakPtr<PlaylistActionIconView> action_icon_view_;  // Our anchor.
  base::WeakPtr<PlaylistTabHelper> tab_helper_;
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
