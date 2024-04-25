/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"

#include "base/logging.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/vector_icon_types.h"

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

void PlaylistActionIconView::ShowPlaylistBubble() {
  DVLOG(2) << __FUNCTION__;

  if (playlist::PlaylistActionBubbleView::IsShowingBubble()) {
    return;
  }

  auto* tab_helper = GetPlaylistTabHelper();
  if (!tab_helper) {
    return;
  }

  playlist::PlaylistActionBubbleView::ShowBubble(
      browser_, weak_ptr_factory_.GetWeakPtr(), tab_helper->GetWeakPtr());
}

views::BubbleDialogDelegate* PlaylistActionIconView::GetBubble() const {
  return playlist::PlaylistActionBubbleView::GetBubble();
}

const gfx::VectorIcon& PlaylistActionIconView::GetVectorIcon() const {
  auto* tab_helper = GetPlaylistTabHelper();
  return tab_helper && !tab_helper->saved_items().empty()
             ? kLeoProductPlaylistAddedIcon
             : kLeoProductPlaylistAddIcon;
}

void PlaylistActionIconView::UpdateImpl() {
  if (!GetWebContents()) {
    return;
  }

  playlist::PlaylistActionBubbleView::MaybeCloseBubble();

  tab_helper_observation_.Reset();
  if (auto* tab_helper = GetPlaylistTabHelper()) {
    tab_helper_observation_.Observe(tab_helper);
  }

  UpdateState();
}

void PlaylistActionIconView::PlaylistTabHelperWillBeDestroyed() {
  tab_helper_observation_.Reset();
}

void PlaylistActionIconView::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>&) {
  UpdateState();
}

void PlaylistActionIconView::OnFoundItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>&) {
  UpdateState();
}

void PlaylistActionIconView::OnAddedItemFromTabHelper(
    const std::vector<playlist::mojom::PlaylistItemPtr>&) {
  DVLOG(2) << __FUNCTION__;
  // When this callback is invoked to this by a tab helper, it means that this
  // view is now bound to the tab helper. So we don't have to check it again.
  if (!playlist::PlaylistActionBubbleView::IsShowingBubble()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&PlaylistActionIconView::ShowPlaylistBubble,
                                  weak_ptr_factory_.GetWeakPtr()));
  }
}

const playlist::PlaylistTabHelper*
PlaylistActionIconView::GetPlaylistTabHelper() const {
  if (auto* contents = GetWebContents()) {
    return playlist::PlaylistTabHelper::FromWebContents(contents);
  }

  return nullptr;
}

playlist::PlaylistTabHelper* PlaylistActionIconView::GetPlaylistTabHelper() {
  return const_cast<playlist::PlaylistTabHelper*>(
      std::as_const(*this).GetPlaylistTabHelper());
}

void PlaylistActionIconView::UpdateState() {
  bool has_saved_items = false;
  bool has_found_items = false;
  if (auto* tab_helper = GetPlaylistTabHelper()) {
    has_saved_items = !tab_helper->saved_items().empty();
    has_found_items = !tab_helper->found_items().empty();
  }

  if (auto old_state = std::exchange(state_, has_saved_items   ? State::kSaved
                                             : has_found_items ? State::kFound
                                                               : State::kNone);
      state_ != old_state) {
    UpdateIconImage();
  }

  SetVisible(state_ != State::kNone);
}

BEGIN_METADATA(PlaylistActionIconView);
END_METADATA
