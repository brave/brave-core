/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_

#include <memory>
#include <vector>

#include "base/functional/callback_forward.h"
#include "brave/browser/ui/views/playlist/selectable_list_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;
class PlaylistActionIconView;
class ThumbnailProvider;

namespace playlist {
class PlaylistTabHelper;
}  // namespace playlist

class PlaylistActionBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(PlaylistActionBubbleView, views::BubbleDialogDelegateView)
 public:
  static void ShowBubble(Browser* browser,
                         PlaylistActionIconView* anchor,
                         playlist::PlaylistTabHelper* playlist_tab_helper);
  static bool IsShowingBubble();
  static void CloseBubble();
  static views::BubbleDialogDelegateView* GetBubble();

  ~PlaylistActionBubbleView() override;

  void CloseAndRun(base::OnceClosure callback);

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

////////////////////////////////////////////////////////////////////////////////
// PlaylistActionAddBubble
//  * Shows when users try adding items found from the current contents.
//  * Shows a list of found items and users can select which one to add.
class PlaylistActionAddBubble : public PlaylistActionBubbleView,
                                public playlist::PlaylistTabHelperObserver {
  METADATA_HEADER(PlaylistActionAddBubble, PlaylistActionBubbleView)
 public:
  static constexpr int kWidth = 288;

  PlaylistActionAddBubble(Browser* browser,
                          PlaylistActionIconView* anchor,
                          playlist::PlaylistTabHelper* playlist_tab_helper);
  PlaylistActionAddBubble(
      Browser* browser,
      PlaylistActionIconView* anchor,
      playlist::PlaylistTabHelper* playlist_tab_helper,
      const std::vector<playlist::mojom::PlaylistItemPtr>& items);
  ~PlaylistActionAddBubble() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, AddItemsToList);

  // PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;
  void OnFoundItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override {}
  void OnAddedItemFromTabHelper(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override {}

  void OnMediaExtracted(bool result);
  void InitListView();

  bool AddSelected();
  void OnSelectionChanged();

  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<SelectableItemsView> list_view_ = nullptr;
  raw_ptr<views::View> loading_spinner_ = nullptr;

  std::unique_ptr<ThumbnailProvider> thumbnail_provider_;

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      playlist_tab_helper_observation_{this};

  base::WeakPtrFactory<PlaylistActionAddBubble> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_BUBBLE_VIEW_H_
