// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/playlist_page_action_controller.h"

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/models/image_model.h"
#include "ui/views/bubble/bubble_anchor.h"
#include "ui/views/view.h"

namespace page_actions {

PlaylistPageActionController::PlaylistPageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {}

PlaylistPageActionController::~PlaylistPageActionController() = default;

void PlaylistPageActionController::Init() {
  playlist_enabled_.Init(
      playlist::kPlaylistEnabledPref,
      user_prefs::UserPrefs::Get(tab_->GetContents()->GetBrowserContext()),
      base::BindRepeating(&PlaylistPageActionController::UpdatePageAction,
                          base::Unretained(this)));

  did_activate_subscription_ = tab_->RegisterDidActivate(base::BindRepeating(
      [](PlaylistPageActionController* self, tabs::TabInterface*) {
        self->AttachToTabHelper();
        self->UpdateState();
      },
      base::Unretained(this)));
  will_discard_contents_subscription_ =
      tab_->RegisterWillDiscardContents(base::BindRepeating(
          [](PlaylistPageActionController* self, tabs::TabInterface*,
             content::WebContents*, content::WebContents*) {
            self->AttachToTabHelper();
            self->UpdateState();
          },
          base::Unretained(this)));
  AttachToTabHelper();
  UpdateState();
}

void PlaylistPageActionController::ExecuteAction() {
  ShowBubble(playlist::PlaylistBubblesController::BubbleType::kInfer);
}

void PlaylistPageActionController::PlaylistTabHelperWillBeDestroyed() {
  tab_helper_observation_.Reset();
}

void PlaylistPageActionController::OnSavedItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  UpdateState();
}

void PlaylistPageActionController::OnFoundItemsChanged(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  UpdateState();
}

void PlaylistPageActionController::OnAddedItemFromTabHelper(
    const std::vector<playlist::mojom::PlaylistItemPtr>& items) {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    return;
  }
  auto* controller =
      playlist::PlaylistBubblesController::CreateOrGetFromWebContents(contents);
  if (!controller->GetBubble()) {
    ShowBubble(playlist::PlaylistBubblesController::BubbleType::kInfer);
  }
}

void PlaylistPageActionController::AttachToTabHelper() {
  tab_helper_observation_.Reset();
  if (content::WebContents* contents = tab_->GetContents()) {
    if (auto* tab_helper =
            playlist::PlaylistTabHelper::FromWebContents(contents)) {
      tab_helper_observation_.Observe(tab_helper);
    }
  }
}

void PlaylistPageActionController::ShowBubble(
    playlist::PlaylistBubblesController::BubbleType type) {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    return;
  }

  BrowserWindowInterface* const bwi = tab_->GetBrowserWindowInterface();
  if (!bwi) {
    return;
  }

  BrowserView& browser_view =
      CHECK_DEREF(BrowserView::GetBrowserViewForBrowser(bwi));
  views::View* const anchor_view =
      browser_view.toolbar_button_provider()
          ->GetPageActionBubbleAnchor(kActionShowPlaylistPageAction)
          .GetIfView();
  if (!anchor_view || !anchor_view->GetWidget()) {
    return;
  }

  playlist::PlaylistBubblesController::CreateOrGetFromWebContents(contents)
      ->ShowBubble(anchor_view, type);
}

void PlaylistPageActionController::UpdateState() {
  bool has_saved_items = false;
  bool has_found_items = false;
  if (content::WebContents* contents = tab_->GetContents()) {
    if (auto* tab_helper =
            playlist::PlaylistTabHelper::FromWebContents(contents)) {
      has_saved_items = !tab_helper->saved_items().empty();
      has_found_items = !tab_helper->found_items().empty();
    }
  }

  state_ = has_saved_items   ? State::kSaved
           : has_found_items ? State::kFound
                             : State::kNone;
  UpdatePageAction();
}

void PlaylistPageActionController::UpdatePageAction() {
  if (!playlist_enabled_.GetValue() || state_ == State::kNone) {
    page_action_controller_->Hide(kActionShowPlaylistPageAction);
    return;
  }

  page_action_controller_->Show(kActionShowPlaylistPageAction);
  page_action_controller_->OverrideImage(
      kActionShowPlaylistPageAction,
      ui::ImageModel::FromVectorIcon(state_ == State::kSaved
                                         ? kLeoProductPlaylistAddedIcon
                                         : kLeoProductPlaylistAddIcon,
                                     ui::kColorIcon));
}

}  // namespace page_actions
