/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ADD_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ADD_BUBBLE_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "brave/browser/ui/views/playlist/selectable_list_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;
class PlaylistActionIconView;
class ThumbnailProvider;

namespace views {
class ScrollView;
class View;
}  // namespace views

namespace playlist {
class PlaylistTabHelper;

// Shows when users try adding items found from the current contents.
// Shows a list of found items and users can select which one to add.
class PlaylistAddBubble : public PlaylistActionBubbleView,
                          public PlaylistTabHelperObserver {
  METADATA_HEADER(PlaylistAddBubble, PlaylistActionBubbleView)
 public:
  static constexpr int kWidth = 288;

  PlaylistAddBubble(Browser* browser,
                    base::WeakPtr<PlaylistActionIconView> action_icon_view,
                    base::WeakPtr<PlaylistTabHelper> tab_helper);
  PlaylistAddBubble(Browser* browser,
                    base::WeakPtr<PlaylistActionIconView> action_icon_view,
                    base::WeakPtr<PlaylistTabHelper> tab_helper,
                    const std::vector<mojom::PlaylistItemPtr>& items);
  ~PlaylistAddBubble() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, AddItemsToList);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTestWithSitesUsingMediaSource,
                           MediaShouldBeExtractedFromBackground_FailToExtract);

  // PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnAddedItemFromTabHelper(
      const std::vector<mojom::PlaylistItemPtr>& items) override;

  void InitListView();

  bool AddSelected();
  void OnSelectionChanged();

  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<SelectableItemsView> list_view_ = nullptr;
  raw_ptr<views::View> loading_spinner_ = nullptr;

  std::unique_ptr<ThumbnailProvider> thumbnail_provider_;

  base::ScopedObservation<PlaylistTabHelper, PlaylistTabHelperObserver>
      tab_helper_observation_{this};

  base::WeakPtrFactory<PlaylistAddBubble> weak_ptr_factory_{this};
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ADD_BUBBLE_H_
