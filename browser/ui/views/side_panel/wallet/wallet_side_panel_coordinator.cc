/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_coordinator.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/views/side_panel/wallet/wallet_side_panel_contents_wrapper.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "ui/base/page_transition_types.h"

WalletSidePanelCoordinator::WalletSidePanelCoordinator(
    BrowserWindowInterface* browser,
    Profile* profile)
    : browser_(browser), profile_(profile) {
  browser_->GetTabStripModel()->AddObserver(this);
}

WalletSidePanelCoordinator::~WalletSidePanelCoordinator() {
  browser_->GetTabStripModel()->RemoveObserver(this);

  // Explicitly deregister so the registry doesn't hold a stale callback
  // pointing at this destroyed coordinator.
  if (registry_) {
    registry_->Deregister(SidePanelEntry::Key(SidePanelEntry::Id::kWallet));
  }
}

void WalletSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  registry_ = global_registry;
  // Use a weak pointer via lambda instead of base::Unretained(this) so the
  // callback safely no-ops if the coordinator is destroyed before the
  // registry invokes it (e.g. during teardown).
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Key(SidePanelEntry::Id::kWallet),
      base::BindRepeating(
          [](base::WeakPtr<WalletSidePanelCoordinator> coordinator,
             SidePanelEntryScope& scope) -> std::unique_ptr<views::View> {
            return coordinator ? coordinator->CreateWebView(scope) : nullptr;
          },
          weak_ptr_factory_.GetWeakPtr()),
      /*default_content_width_callback=*/base::NullCallback()));
}

void WalletSidePanelCoordinator::Navigate(const GURL& url) {
  if (!contents_wrapper_) {
    return;
  }
  if (!url.SchemeIs(content::kChromeUIScheme) ||
      url.host() != kWalletPanelHost) {
    return;
  }
  contents_wrapper_->web_contents()->GetController().LoadURL(
      url, content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
  // LoadURL creates a new WebUI instance, so re-bind the embedder so the
  // new WalletPanelUI can communicate back to its host (e.g. ShowUI/CloseUI).
  if (auto* controller = contents_wrapper_->GetWebUIController()) {
    controller->set_embedder(contents_wrapper_->GetWeakPtr());
  }
}

std::unique_ptr<views::View> WalletSidePanelCoordinator::CreateWebView(
    SidePanelEntryScope& scope) {
  // Reuse the existing contents wrapper if the panel was previously shown
  // in this session. This preserves wallet state across panel toggles.
  if (!contents_wrapper_) {
    contents_wrapper_ = std::make_unique<WalletSidePanelContentsWrapper>(
        GURL(kBraveUIWalletPanelURL), profile_, IDS_SIDEBAR_WALLET_ITEM_TITLE,
        /*esc_closes_ui=*/false);
    contents_wrapper_->ReloadWebContents();
  } else {
    contents_wrapper_->web_contents()->UpdateWebContentsVisibility(
        content::Visibility::VISIBLE);
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
  contents_wrapper_.reset();
}

void WalletSidePanelCoordinator::WillCloseAllTabs(
    TabStripModel* tab_strip_model) {
  // WalletPanelUI holds a raw_ptr to the active tab's WebContents. Destroy the
  // contents wrapper (and with it WalletPanelUI) before the tabs are closed to
  // prevent a dangling pointer.
  contents_wrapper_.reset();
}
