// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PLAYLIST_PAGE_ACTION_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PLAYLIST_PAGE_ACTION_CONTROLLER_H_

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/components/playlist/content/browser/playlist_tab_helper.h"
#include "brave/components/playlist/content/browser/playlist_tab_helper_observer.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "components/prefs/pref_member.h"
#include "components/tabs/public/tab_interface.h"

namespace page_actions {

// Drives the Playlist page action: shows an icon when the current page has
// saved or found playlist items, and shows the Playlist bubble when clicked
// (or automatically, right after an item is added from the web page itself).
class PlaylistPageActionController
    : public playlist::PlaylistTabHelperObserver {
 public:
  PlaylistPageActionController(
      tabs::TabInterface& tab,
      page_actions::PageActionController& page_action_controller);
  PlaylistPageActionController(const PlaylistPageActionController&) = delete;
  PlaylistPageActionController& operator=(const PlaylistPageActionController&) =
      delete;
  ~PlaylistPageActionController() override;

  void Init();

  void ExecuteAction();

  // Shows the Playlist bubble with an explicit bubble type. Used both by
  // ExecuteAction() (kInfer) and by BraveLocationBarView::ShowPlaylistBubble()
  // (IDC_SHOW_PLAYLIST_BUBBLE, e.g. a keyboard shortcut).
  void ShowBubble(playlist::PlaylistBubblesController::BubbleType type);

 private:
  // playlist::PlaylistTabHelperObserver:
  void PlaylistTabHelperWillBeDestroyed() override;
  void OnSavedItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;
  void OnFoundItemsChanged(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;
  void OnAddedItemFromTabHelper(
      const std::vector<playlist::mojom::PlaylistItemPtr>& items) override;

  // (Re-)observes the current WebContents' PlaylistTabHelper, since the tab's
  // contents can be swapped out (e.g. tab discarding).
  void AttachToTabHelper();

  void UpdateState();
  void UpdatePageAction();

  enum class State { kNone, kSaved, kFound } state_ = State::kNone;

  const raw_ref<tabs::TabInterface> tab_;
  const raw_ref<page_actions::PageActionControllerImpl> page_action_controller_;

  BooleanPrefMember playlist_enabled_;

  base::CallbackListSubscription did_activate_subscription_;
  base::CallbackListSubscription will_discard_contents_subscription_;

  base::ScopedObservation<playlist::PlaylistTabHelper,
                          playlist::PlaylistTabHelperObserver>
      tab_helper_observation_{this};
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_PLAYLIST_PAGE_ACTION_CONTROLLER_H_
