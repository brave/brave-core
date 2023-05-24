/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"

PlaylistActionIconView::PlaylistActionIconView(
    CommandUpdater* command_updater,
    Browser* browser,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(command_updater,
                         0,
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

void PlaylistActionIconView::OnExecuting(ExecuteSource execute_source) {
  if (PlaylistActionBubbleView::IsShowingBubble()) {
    return;
  }

  DCHECK_EQ(current_web_contents_, GetWebContents());
  PlaylistActionBubbleView::ShowBubble(this,
                                       base::to_address(current_web_contents_));
}

const gfx::VectorIcon& PlaylistActionIconView::GetVectorIcon() const {
  return state_ == kAdded ? kPlaylistActionAddedIcon : kPlaylistActionIcon;
}

void PlaylistActionIconView::UpdateImpl() {
  if (auto old_contents =
          std::exchange(current_web_contents_, GetWebContents());
      old_contents == current_web_contents_) {
    return;
  }

  if (PlaylistActionBubbleView::IsShowingBubble()) {
    PlaylistActionBubbleView::CloseBubble();
  }

  auto* playlist_tab_helper = this->playlist_tab_helper();
  if (!playlist_tab_helper) {
    UpdateState(0, 0);
    return;
  }

  saved_items_changed_subscription_ =
      playlist_tab_helper->RegisterSavedItemsChangedCallback(
          base::BindRepeating(&PlaylistActionIconView::OnSavedItemChanged,
                              base::Unretained(this)));
  found_items_changed_subscription_ =
      playlist_tab_helper->RegisterFoundItemsChangedCallback(
          base::BindRepeating(&PlaylistActionIconView::OnFoundItemChanged,
                              base::Unretained(this)));

  UpdateState(playlist_tab_helper->saved_items().size(),
              playlist_tab_helper->found_items().size());
}

void PlaylistActionIconView::OnSavedItemChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& saved_items) {
  auto* playlist_tab_helper = this->playlist_tab_helper();
  CHECK(playlist_tab_helper);

  UpdateState(saved_items.size(), playlist_tab_helper->found_items().size());
}

void PlaylistActionIconView::OnFoundItemChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& found_items) {
  auto* playlist_tab_helper = this->playlist_tab_helper();
  CHECK(playlist_tab_helper);

  UpdateState(playlist_tab_helper->saved_items().size(), found_items.size());
}

void PlaylistActionIconView::UpdateState(int saved_items_count,
                                         int found_items_count) {
  State target_state = saved_items_count   ? kAdded
                       : found_items_count ? kFound
                                           : kNone;
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
