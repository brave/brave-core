/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_EDIT_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_EDIT_BUBBLE_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/playlist/playlist_bubble_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;

namespace views {
class View;
}

namespace playlist {
// Shows when items were added to the current page.
// Contains actions to manipulate items.
class PlaylistEditBubbleView : public PlaylistBubbleView,
                              public PlaylistTabHelperObserver {
  METADATA_HEADER(PlaylistEditBubbleView, PlaylistBubbleView)
 public:
  PlaylistEditBubbleView(views::View* anchor_view,
                        base::WeakPtr<PlaylistTabHelper> tab_helper);
  ~PlaylistEditBubbleView() override;

  // PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(
      const std::vector<mojom::PlaylistItemPtr>& items) override;

 private:
  void ResetChildViews();

  void OpenInPlaylist();
  void ChangeFolder();
  void RemoveFromPlaylist();
  void MoreMediaInContents();

  raw_ptr<Browser> browser_;
  base::ScopedObservation<PlaylistTabHelper, PlaylistTabHelperObserver>
      tab_helper_observation_{this};
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_EDIT_BUBBLE_VIEW_H_
