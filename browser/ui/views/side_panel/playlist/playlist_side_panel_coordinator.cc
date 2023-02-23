/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"

#include <memory>

#include "base/functional/callback.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_web_view.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/views/vector_icons.h"

PlaylistSidePanelCoordinator::PlaylistSidePanelCoordinator(Browser* browser)
    : BrowserUserData<PlaylistSidePanelCoordinator>(*browser) {}

PlaylistSidePanelCoordinator::~PlaylistSidePanelCoordinator() = default;

void PlaylistSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Id::kPlaylist,
      l10n_util::GetStringUTF16(IDS_SIDEBAR_PLAYLIST_ITEM_TITLE),
      ui::ImageModel(),
      base::BindRepeating(&PlaylistSidePanelCoordinator::CreateWebView,
                          base::Unretained(this))));
}

std::unique_ptr<views::View> PlaylistSidePanelCoordinator::CreateWebView() {
  const bool should_create_contents_wrapper = !contents_wrapper_;
  if (should_create_contents_wrapper) {
    contents_wrapper_ =
        std::make_unique<BubbleContentsWrapperT<playlist::PlaylistUI>>(
            GURL(kPlaylistURL), GetBrowser().profile(),
            IDS_SIDEBAR_PLAYLIST_ITEM_TITLE,
            /*webui_resizes_host=*/false,
            /*esc_closes_ui=*/false);
    contents_wrapper_->ReloadWebContents();
  }

  auto web_view = std::make_unique<PlaylistSidePanelWebView>(
      &GetBrowser(), base::DoNothing(), contents_wrapper_.get());
  if (!should_create_contents_wrapper) {
    // SidePanelWebView's initial visibility is hidden. Thus, we need to
    // call this manually when we don't reload the web contents.
    // Calling this will also mark that the web contents is ready to go.
    web_view->ShowUI();
  }

  view_observation_.Observe(web_view.get());

  return web_view;
}

void PlaylistSidePanelCoordinator::OnViewIsDeleting(views::View* view) {
  DestroyWebContentsIfNeeded();

  view_observation_.Reset();
}

void PlaylistSidePanelCoordinator::DestroyWebContentsIfNeeded() {
  DCHECK(contents_wrapper_);
  if (!contents_wrapper_->web_contents()->GetCurrentlyPlayingVideoCount())
    contents_wrapper_.reset();
}

BROWSER_USER_DATA_KEY_IMPL(PlaylistSidePanelCoordinator);
