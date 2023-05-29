/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"

#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
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
                         /*ephemeral=*/false) {
  SetVisible(false);
}

PlaylistActionIconView::~PlaylistActionIconView() = default;

views::BubbleDialogDelegate* PlaylistActionIconView::GetBubble() const {
  return PlaylistActionBubbleView::GetBubble();
}

void PlaylistActionIconView::ShowPlaylistBubble() {
  auto* current_contents = GetWebContents();
  DCHECK_EQ(last_web_contents_, current_contents);

  if (PlaylistActionBubbleView::IsShowingBubble() || !current_contents) {
    return;
  }
  PlaylistActionBubbleView::ShowBubble(this, current_contents);
}

const gfx::VectorIcon& PlaylistActionIconView::GetVectorIcon() const {
  return state_ == kAdded ? kLeoProductPlaylistAddedIcon
                          : kLeoProductPlaylistAddIcon;
}

void PlaylistActionIconView::UpdateImpl() {
  if (auto old_contents = std::exchange(last_web_contents_, GetWebContents());
      old_contents == last_web_contents_) {
    return;
  }

  if (PlaylistActionBubbleView::IsShowingBubble()) {
    PlaylistActionBubbleView::CloseBubble();
  }
  playlist_tab_helper_observation_.Reset();

  auto* playlist_tab_helper = this->playlist_tab_helper();
  if (!playlist_tab_helper) {
    UpdateState(/* has_saved= */ false, /* found_items= */ false);
    return;
  }

  playlist_tab_helper_observation_.Observe(playlist_tab_helper);

  UpdateState(playlist_tab_helper->saved_items().size(),
              playlist_tab_helper->found_items().size());
}

void PlaylistActionIconView::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& saved_items) {
  auto* playlist_tab_helper = this->playlist_tab_helper();
  CHECK(playlist_tab_helper);

  UpdateState(saved_items.size(), playlist_tab_helper->found_items().size());
}

void PlaylistActionIconView::OnFoundItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& found_items) {
  auto* playlist_tab_helper = this->playlist_tab_helper();
  CHECK(playlist_tab_helper);

  UpdateState(playlist_tab_helper->saved_items().size(), found_items.size());
}

void PlaylistActionIconView::UpdateState(bool has_saved, bool found_items) {
  State target_state = has_saved ? kAdded : found_items ? kFound : kNone;
  if (auto old_state = std::exchange(state_, target_state);
      old_state == target_state) {
    return;
  }

  UpdateIconImage();
  UpdateVisibilityPerState();
}

void PlaylistActionIconView::UpdateVisibilityPerState() {
  const bool should_be_visible = state_ != kNone;
  if (GetVisible() == should_be_visible) {
    return;
  }

  SetVisible(should_be_visible);
  PreferredSizeChanged();
}

BEGIN_METADATA(PlaylistActionIconView, PageActionIconView);
END_METADATA
