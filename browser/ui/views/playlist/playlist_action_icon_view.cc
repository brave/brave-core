/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"

#include <utility>

#include "base/logging.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "ui/base/metadata/metadata_impl_macros.h"

PlaylistActionIconView::PlaylistActionIconView(
    CommandUpdater* command_updater,
    Browser* browser,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(command_updater,
                         IDC_SHOW_PLAYLIST_BUBBLE,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate,
                         "PlaylistActionIconView",
                         /*ephemeral=*/false),
      browser_(browser) {
  SetVisible(false);
}

PlaylistActionIconView::~PlaylistActionIconView() = default;

views::BubbleDialogDelegate* PlaylistActionIconView::GetBubble() const {
  return PlaylistActionBubbleView::GetBubble();
}

void PlaylistActionIconView::ShowPlaylistBubble() {
  DVLOG(2) << __FUNCTION__;
  if (state_ == State::kNone) {
    return;
  }

  if (PlaylistActionBubbleView::IsShowingBubble()) {
    return;
  }

  auto* playlist_tab_helper = GetPlaylistTabHelper();
  if (!playlist_tab_helper || playlist_tab_helper->is_adding_items()) {
    return;
  }

  const auto saved_item_count = playlist_tab_helper->saved_items().size();
  const auto found_item_count = playlist_tab_helper->found_items().size();
  if (!saved_item_count && !found_item_count) {
    return;
  }

  DCHECK(saved_item_count || found_item_count);
  PlaylistActionBubbleView::ShowBubble(browser_, this, playlist_tab_helper);
}

base::WeakPtr<PlaylistActionIconView> PlaylistActionIconView::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

const gfx::VectorIcon& PlaylistActionIconView::GetVectorIcon() const {
  return state_ == State::kAdded ? kLeoProductPlaylistAddedIcon
                                 : kLeoProductPlaylistAddIcon;
}

void PlaylistActionIconView::UpdateImpl() {
  if (!GetWebContents()) {
    return;
  }

  if (PlaylistActionBubbleView::IsShowingBubble()) {
    PlaylistActionBubbleView::CloseBubble();
  }
  playlist_tab_helper_observation_.Reset();

  auto* playlist_tab_helper = GetPlaylistTabHelper();
  if (!playlist_tab_helper) {
    UpdateState(/* has_saved= */ false, /* found_items= */ false);
    return;
  }

  playlist_tab_helper_observation_.Observe(playlist_tab_helper);

  UpdateState(playlist_tab_helper->saved_items().size(),
              playlist_tab_helper->found_items().size());
}

void PlaylistActionIconView::PlaylistTabHelperWillBeDestroyed() {
  playlist_tab_helper_observation_.Reset();
}

void PlaylistActionIconView::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& saved_items) {
  auto* playlist_tab_helper = GetPlaylistTabHelper();
  if (!playlist_tab_helper) {
    return;
  }

  UpdateState(saved_items.size(), playlist_tab_helper->found_items().size());
}

void PlaylistActionIconView::OnFoundItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& found_items) {
  auto* playlist_tab_helper = GetPlaylistTabHelper();
  if (!playlist_tab_helper) {
    return;
  }

  UpdateState(playlist_tab_helper->saved_items().size(), found_items.size());
}

playlist::PlaylistTabHelper* PlaylistActionIconView::GetPlaylistTabHelper() {
  if (auto* contents = GetWebContents()) {
    return playlist::PlaylistTabHelper::FromWebContents(contents);
  }

  return nullptr;
}

void PlaylistActionIconView::UpdateState(bool has_saved, bool found_items) {
  State target_state = has_saved     ? State::kAdded
                       : found_items ? State::kFound
                                     : State::kNone;
  if (auto old_state = std::exchange(state_, target_state);
      old_state != target_state) {
    DVLOG(2) << __FUNCTION__ << " " << static_cast<int>(target_state);
    UpdateIconImage();
  }
  UpdateVisibilityPerState();
}

void PlaylistActionIconView::UpdateVisibilityPerState() {
  const bool should_be_visible = state_ != State::kNone;
  if (GetVisible() == should_be_visible) {
    return;
  }

  SetVisible(should_be_visible);
  PreferredSizeChanged();
}

void PlaylistActionIconView::OnAddedItemFromTabHelper(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  DVLOG(2) << __FUNCTION__;
  // When this callback is invoked to this by a tab helper, it means that this
  // view is now bound to the tab helper. So we don't have to check it again.
  if (!PlaylistActionBubbleView::IsShowingBubble()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&PlaylistActionIconView::ShowPlaylistBubble,
                                  weak_ptr_factory_.GetWeakPtr()));
  }
}

BEGIN_METADATA(PlaylistActionIconView, PageActionIconView);
END_METADATA
