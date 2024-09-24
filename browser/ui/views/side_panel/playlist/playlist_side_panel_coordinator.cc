/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"

#include <memory>

#include "base/functional/callback.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/views/vector_icons.h"

PlaylistSidePanelCoordinator::Proxy::Proxy(
    content::WebContents* web_contents,
    base::WeakPtr<PlaylistSidePanelCoordinator> coordinator)
    : WebContentsUserData(*web_contents), coordinator_(coordinator) {}

PlaylistSidePanelCoordinator::Proxy::~Proxy() = default;

base::WeakPtr<PlaylistSidePanelCoordinator>
PlaylistSidePanelCoordinator::Proxy::GetCoordinator() {
  return coordinator_;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistSidePanelCoordinator::Proxy);

PlaylistSidePanelCoordinator::PlaylistSidePanelCoordinator(Browser* browser)
    : BrowserUserData<PlaylistSidePanelCoordinator>(*browser),
      browser_(browser) {}

PlaylistSidePanelCoordinator::~PlaylistSidePanelCoordinator() = default;

void PlaylistSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Id::kPlaylist,
      base::BindRepeating(&PlaylistSidePanelCoordinator::CreateWebView,
                          base::Unretained(this))));
}

void PlaylistSidePanelCoordinator::ActivatePanel() {
  auto* sidebar_controller =
      static_cast<BraveBrowser*>(browser_.get())->sidebar_controller();
  sidebar_controller->ActivatePanelItem(
      sidebar::SidebarItem::BuiltInItemType::kPlaylist);
}

void PlaylistSidePanelCoordinator::LoadPlaylist(const std::string& playlist_id,
                                                const std::string& item_id) {
  CHECK(contents_wrapper());

  auto* web_contents = contents_wrapper()->web_contents();
  CHECK(web_contents);

  CHECK(!playlist_id.empty());
  CHECK(web_contents->GetController().LoadURL(
      GURL("chrome-untrusted://playlist/playlist/" + playlist_id + "#" +
           item_id),
      {}, ui::PageTransition::PAGE_TRANSITION_AUTO_BOOKMARK, {}));
}

std::unique_ptr<views::View> PlaylistSidePanelCoordinator::CreateWebView() {
  const bool should_create_contents_wrapper = !contents_wrapper_;
  if (should_create_contents_wrapper) {
    contents_wrapper_ = std::make_unique<PlaylistContentsWrapper>(
        GURL(kPlaylistURL), GetBrowser().profile(),
        IDS_SIDEBAR_PLAYLIST_ITEM_TITLE,
        /*esc_closes_ui=*/false,
        static_cast<BrowserView*>(GetBrowser().window()), this);
    contents_wrapper_->ReloadWebContents();

    Proxy::CreateForWebContents(contents_wrapper_->web_contents(),
                                weak_ptr_factory_.GetWeakPtr());
  }

  auto web_view = std::make_unique<PlaylistSidePanelWebView>(
      &GetBrowser(), base::DoNothing(), contents_wrapper_.get());
  side_panel_web_view_ = web_view->GetWeakPtr();

  if (!should_create_contents_wrapper) {
    // SidePanelWebView's initial visibility is hidden. Thus, we need to
    // call this manually when we don't reload the web contents.
    // Calling this will also mark that the web contents is ready to go.
    web_view->ShowUI();
  }

  view_observation_.Observe(web_view.get());

  return web_view;
}

BrowserView* PlaylistSidePanelCoordinator::GetBrowserView() {
  return static_cast<BrowserView*>(GetBrowser().window());
}

void PlaylistSidePanelCoordinator::OnViewIsDeleting(views::View* view) {
  DestroyWebContentsIfNeeded();

  view_observation_.Reset();
}

void PlaylistSidePanelCoordinator::DestroyWebContentsIfNeeded() {
  DCHECK(contents_wrapper_);
  if (!contents_wrapper_->web_contents()->IsCurrentlyAudible()) {
    contents_wrapper_.reset();
  }
}

BROWSER_USER_DATA_KEY_IMPL(PlaylistSidePanelCoordinator);
