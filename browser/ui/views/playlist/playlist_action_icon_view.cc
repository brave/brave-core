/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"

#include "base/logging.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/views/playlist/playlist_bubble_view.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/user_prefs/user_prefs.h"
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
                         /*ephemeral=*/false) {
  playlist_enabled_.Init(
      playlist::kPlaylistEnabledPref,
      user_prefs::UserPrefs::Get(browser->profile()),
      base::BindRepeating(&PlaylistActionIconView::UpdateState,
                          weak_ptr_factory_.GetWeakPtr()));
  SetVisible(false);
}

PlaylistActionIconView::~PlaylistActionIconView() = default;

void PlaylistActionIconView::ShowPlaylistBubble(
    playlist::PlaylistBubblesController::BubbleType type) {
  DVLOG(2) << __FUNCTION__;

  if (auto* controller = GetController()) {
    controller->ShowBubble(AsWeakPtr(), type);
  }
}

base::WeakPtr<PlaylistActionIconView> PlaylistActionIconView::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PlaylistActionIconView::SetVisible(bool visible) {
  PageActionIconView::SetVisible(visible && *playlist_enabled_);
}

views::BubbleDialogDelegate* PlaylistActionIconView::GetBubble() const {
  auto* controller = GetController();
  return controller ? controller->GetBubble() : nullptr;
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

  if (auto* controller = GetController();
      controller && !controller->GetBubble()) {
    ShowPlaylistBubble();
  }
}

playlist::PlaylistBubblesController* PlaylistActionIconView::GetController()
    const {
  auto* web_contents = GetWebContents();
  return web_contents
             ? playlist::PlaylistBubblesController::CreateOrGetFromWebContents(
                   web_contents)
             : nullptr;
}

const playlist::PlaylistTabHelper*
PlaylistActionIconView::GetPlaylistTabHelper() const {
  auto* web_contents = GetWebContents();
  return web_contents
             ? playlist::PlaylistTabHelper::FromWebContents(web_contents)
             : nullptr;
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
