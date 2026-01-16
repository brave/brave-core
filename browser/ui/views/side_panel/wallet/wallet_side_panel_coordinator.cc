/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_coordinator.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"

WalletSidePanelCoordinator::WalletSidePanelCoordinator(
    BrowserWindowInterface* browser,
    sidebar::SidebarController* sidebar_controller,
    Profile* profile)
    : browser_(browser),
      sidebar_controller_(sidebar_controller),
      profile_(profile) {}

WalletSidePanelCoordinator::~WalletSidePanelCoordinator() {
  if (registry_) {
    registry_->Deregister(SidePanelEntry::Key(SidePanelEntry::Id::kWallet));
  }
}

void WalletSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  registry_ = global_registry;
  // base::Unretained is safe here because:
  // 1. The entry is deregistered in the destructor, ensuring the callback
  //    won't be invoked after this object is destroyed
  // 2. WeakPtr cannot be used with methods that return values
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Key(SidePanelEntry::Id::kWallet),
      base::BindRepeating(&WalletSidePanelCoordinator::CreateWebView,
                          base::Unretained(this)),
      /*default_content_width_callback=*/base::NullCallback()));
}

void WalletSidePanelCoordinator::ActivatePanel() {
  sidebar_controller_->ActivatePanelItem(
      sidebar::SidebarItem::BuiltInItemType::kWallet);
}

std::unique_ptr<views::View> WalletSidePanelCoordinator::CreateWebView(
    SidePanelEntryScope& scope) {
  if (!contents_wrapper_) {
    contents_wrapper_ = std::make_unique<WebUIContentsWrapperT<WalletPageUI>>(
        GURL(kBraveUIWalletURL), profile_, IDS_SIDEBAR_WALLET_ITEM_TITLE,
        /*esc_closes_ui=*/false);
    contents_wrapper_->ReloadWebContents();
  }

  auto web_view = std::make_unique<SidePanelWebUIView>(
      scope, /*on_show_cb=*/base::RepeatingClosure(),
      /*close_cb=*/base::DoNothing(), contents_wrapper_.get());
  side_panel_web_view_ = web_view.get();

  web_view->ShowUI();

  view_observation_.Observe(web_view.get());

  return web_view;
}

void WalletSidePanelCoordinator::OnViewIsDeleting(views::View* view) {
  view_observation_.Reset();
  side_panel_web_view_ = nullptr;
  // Reset the contents wrapper when the view is deleted to ensure fresh state
  // next time the panel is opened.
  contents_wrapper_.reset();
}
