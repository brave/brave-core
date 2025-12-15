/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"

#include <memory>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
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

PlaylistSidePanelCoordinator::PlaylistSidePanelCoordinator(
    BrowserWindowInterface* browser,
    sidebar::SidebarController* sidebar_controller,
    Profile* profile)
    : browser_(browser),
      sidebar_controller_(sidebar_controller),
      profile_(profile) {}

PlaylistSidePanelCoordinator::~PlaylistSidePanelCoordinator() = default;

void PlaylistSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Key(SidePanelEntry::Id::kPlaylist),
      base::BindRepeating(&PlaylistSidePanelCoordinator::CreateWebView,
                          base::Unretained(this)),
      /*default_content_width_callback=*/base::NullCallback()));
}

void PlaylistSidePanelCoordinator::ActivatePanel() {
  sidebar_controller_->ActivatePanelItem(
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

std::unique_ptr<views::View> PlaylistSidePanelCoordinator::CreateWebView(
    SidePanelEntryScope& scope) {
  const bool should_create_contents_wrapper = !contents_wrapper_;
  if (should_create_contents_wrapper) {
    contents_wrapper_ = std::make_unique<PlaylistContentsWrapper>(
        GURL(kPlaylistURL), profile_, IDS_SIDEBAR_PLAYLIST_ITEM_TITLE,
        /*esc_closes_ui=*/false,
        BrowserView::GetBrowserViewForBrowser(
            browser_->GetBrowserForMigrationOnly()),
        this);
    contents_wrapper_->ReloadWebContents();

    Proxy::CreateForWebContents(contents_wrapper_->web_contents(),
                                weak_ptr_factory_.GetWeakPtr());
  } else {
    // Set visible to avoid CHECK(page_node->IsVisible()) failure in
    // SidePanelLoadingVoter::MarkAsSidePanel(). When SidePanelWebView is
    // created below, upstream marks this content and has assumption that it's
    // visible if it has loaded url. When playlist panel is closed while
    // playing, we cache |contents_wrapper_| to make it continue to play after
    // closing panel. So, it has loaded url already. Should be visible before
    // creating SidePanelWebView with it to avoid above CHECK failure.
    contents_wrapper_->web_contents()->UpdateWebContentsVisibility(
        content::Visibility::VISIBLE);
  }

  auto web_view = std::make_unique<SidePanelWebUIView>(
      scope, /*on_show_cb=*/base::RepeatingClosure(),
      /*close_cb=*/base::DoNothing(), contents_wrapper_.get());
  side_panel_web_view_ = web_view.get();

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
  return BrowserView::GetBrowserViewForBrowser(
      browser_->GetBrowserForMigrationOnly());
}

void PlaylistSidePanelCoordinator::OnViewIsDeleting(views::View* view) {
  view_observation_.Reset();

  DCHECK(contents_wrapper_);
  side_panel_web_view_ = nullptr;

  if (is_audible_for_testing_) {
    CHECK_IS_TEST();
    return;
  }

  if (!contents_wrapper_->web_contents()->IsCurrentlyAudible()) {
    contents_wrapper_.reset();
  }
}
