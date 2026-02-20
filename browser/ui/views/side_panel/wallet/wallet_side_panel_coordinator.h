/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "url/gurl.h"

class BrowserWindowInterface;
class Profile;
class SidePanelRegistry;
class SidePanelEntryScope;
class SidePanelWebUIView;
class WalletSidePanelContentsWrapper;

// Manages the lifecycle and registration of the Wallet side panel entry.
// Uses WalletPanelUI (the compact panel interface) rather than the full-page
// WalletPageUI, hosted in a custom contents wrapper that disables auto-resize
// so the panel fills the available side panel space.
class WalletSidePanelCoordinator : public views::ViewObserver,
                                   public TabStripModelObserver {
 public:
  WalletSidePanelCoordinator(BrowserWindowInterface* browser, Profile* profile);
  WalletSidePanelCoordinator(const WalletSidePanelCoordinator&) = delete;
  WalletSidePanelCoordinator& operator=(const WalletSidePanelCoordinator&) =
      delete;
  ~WalletSidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);

  // Navigates the side panel's web contents to the given URL. Used to route
  // the already-open panel to the correct screen for incoming dapp requests
  // (connection, switch chain, add chain, add token, sign, etc.).
  void Navigate(const GURL& url);

  SidePanelWebUIView* side_panel_web_view() { return side_panel_web_view_; }

  // views::ViewObserver:
  void OnViewIsDeleting(views::View* view) override;

  // TabStripModelObserver:
  void WillCloseAllTabs(TabStripModel* tab_strip_model) override;

 private:
  std::unique_ptr<views::View> CreateWebView(SidePanelEntryScope& scope);

  const raw_ptr<BrowserWindowInterface> browser_;
  const raw_ptr<Profile> profile_;

  raw_ptr<SidePanelRegistry> registry_ = nullptr;

  std::unique_ptr<WalletSidePanelContentsWrapper> contents_wrapper_;

  raw_ptr<SidePanelWebUIView> side_panel_web_view_ = nullptr;

  base::ScopedObservation<views::View, views::ViewObserver> view_observation_{
      this};

  base::WeakPtrFactory<WalletSidePanelCoordinator> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_COORDINATOR_H_
