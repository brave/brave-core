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
  return playlist::PlaylistActionBubbleView::GetBubble();
}

void PlaylistActionIconView::ShowPlaylistBubble() {
  DVLOG(2) << __FUNCTION__;
  if (state_ == State::kNone) {
    return;
  }

  if (playlist::PlaylistActionBubbleView::IsShowingBubble()) {
    return;
  }

  auto* playlist_tab_helper = GetPlaylistTabHelper();
  if (!playlist_tab_helper) {
    return;
  }

  playlist::PlaylistActionBubbleView::ShowBubble(
      browser_, weak_ptr_factory_.GetWeakPtr(),
      playlist_tab_helper->GetWeakPtr());
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

  playlist::PlaylistActionBubbleView::MaybeCloseBubble();

  playlist_tab_helper_observation_.Reset();
  if (auto* tab_helper = GetPlaylistTabHelper()) {
    playlist_tab_helper_observation_.Observe(tab_helper);
  }

  UpdateState();
}

void PlaylistActionIconView::PlaylistTabHelperWillBeDestroyed() {
  playlist_tab_helper_observation_.Reset();
}

void PlaylistActionIconView::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>&) {
  UpdateState();
}

void PlaylistActionIconView::OnFoundItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>&) {
  UpdateState();
}

playlist::PlaylistTabHelper* PlaylistActionIconView::GetPlaylistTabHelper() {
  if (auto* contents = GetWebContents()) {
    return playlist::PlaylistTabHelper::FromWebContents(contents);
  }

  return nullptr;
}

void PlaylistActionIconView::UpdateState() {
  bool has_saved_items = false;
  bool has_found_items = false;
  if (auto* tab_helper = GetPlaylistTabHelper()) {
    has_saved_items = !tab_helper->saved_items().empty();
    has_found_items = !tab_helper->found_items().empty();
  }

  if (auto old_state = std::exchange(state_, has_saved_items   ? State::kAdded
                                             : has_found_items ? State::kFound
                                                               : State::kNone);
      state_ != old_state) {
    DVLOG(2) << __FUNCTION__ << " " << static_cast<int>(state_);
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
  if (!playlist::PlaylistActionBubbleView::IsShowingBubble()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&PlaylistActionIconView::ShowPlaylistBubble,
                                  weak_ptr_factory_.GetWeakPtr()));
  }
}

BEGIN_METADATA(PlaylistActionIconView);
END_METADATA
