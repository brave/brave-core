/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_CONFIRM_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_CONFIRM_BUBBLE_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;
class PlaylistActionIconView;

namespace playlist {
// Shows when items were added to the current page.
// Contains actions to manipulate items.
class PlaylistConfirmBubble : public PlaylistActionBubbleView,
                              public PlaylistTabHelperObserver {
  METADATA_HEADER(PlaylistConfirmBubble, PlaylistActionBubbleView)
 public:
  PlaylistConfirmBubble(Browser* browser,
                        base::WeakPtr<PlaylistActionIconView> action_icon_view,
                        base::WeakPtr<PlaylistTabHelper> tab_helper);
  ~PlaylistConfirmBubble() override;

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

  base::ScopedObservation<PlaylistTabHelper, PlaylistTabHelperObserver>
      tab_helper_observation_{this};
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_CONFIRM_BUBBLE_H_
