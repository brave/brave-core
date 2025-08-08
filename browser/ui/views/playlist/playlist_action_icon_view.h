/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/components/playlist/browser/playlist_tab_helper_observer.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;
class CommandUpdater;

namespace gfx {
struct VectorIcon;
}

namespace playlist {
class PlaylistTabHelper;
}  // namespace playlist

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

  void ShowPlaylistBubble(
      playlist::PlaylistBubblesController::BubbleType type =
          playlist::PlaylistBubblesController::BubbleType::kInfer);
  base::WeakPtr<PlaylistActionIconView> AsWeakPtr();

  // PageActionIconView:
  void SetVisible(bool visible) override;

 private:
  // PageActionIconView:
  void OnExecuting(ExecuteSource execute_source) override {}
  views::BubbleDialogDelegate* GetBubble() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  void UpdateImpl() override;

  // PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(const std::vector<playlist::mojom::PlaylistItemPtr>&
                               saved_items) override;
  void OnFoundItemsChanged(const std::vector<playlist::mojom::PlaylistItemPtr>&
                               found_items) override;
  void OnAddedItemFromTabHelper(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;

  playlist::PlaylistBubblesController* GetController() const;
  const playlist::PlaylistTabHelper* GetPlaylistTabHelper() const;
  playlist::PlaylistTabHelper* GetPlaylistTabHelper();
  void UpdateState();

  BooleanPrefMember playlist_enabled_;

  enum class State { kNone, kSaved, kFound } state_ = State::kNone;

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      tab_helper_observation_{this};

  base::WeakPtrFactory<PlaylistActionIconView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_PLAYLIST_ACTION_ICON_VIEW_H_
