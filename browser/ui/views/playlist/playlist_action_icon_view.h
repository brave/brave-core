/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_

#include <vector>

#include "base/scoped_observation.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"

class Browser;

class PlaylistActionIconView : public PageActionIconView,
                               public playlist::PlaylistTabHelperObserver {
  METADATA_HEADER(PlaylistActionIconView, PageActionIconView)
 public:

  PlaylistActionIconView(
      CommandUpdater* command_updater,
      Browser* browser,
      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
      PageActionIconView::Delegate* page_action_icon_delegate);
  PlaylistActionIconView(const PlaylistActionIconView&) = delete;
  PlaylistActionIconView& operator=(const PlaylistActionIconView&) = delete;
  ~PlaylistActionIconView() override;

  void ShowPlaylistBubble();

  base::WeakPtr<PlaylistActionIconView> GetWeakPtr();

  // PageActionIconView:
  void OnExecuting(ExecuteSource execute_source) override {}
  views::BubbleDialogDelegate* GetBubble() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  void UpdateImpl() override;

 private:
  enum class State { kNone, kAdded, kFound };

  playlist::PlaylistTabHelper* GetPlaylistTabHelper();

  void UpdateState();
  void UpdateVisibilityPerState();

  // PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(const std::vector<playlist::mojom::PlaylistItemPtr>&
                               saved_items) override;
  void OnFoundItemsChanged(const std::vector<playlist::mojom::PlaylistItemPtr>&
                               found_items) override;
  void OnAddedItemFromTabHelper(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;

  raw_ptr<Browser> browser_ = nullptr;

  State state_ = State::kNone;

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      playlist_tab_helper_observation_{this};

  base::WeakPtrFactory<PlaylistActionIconView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_
