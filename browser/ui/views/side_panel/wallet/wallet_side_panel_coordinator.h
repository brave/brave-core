/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserWindowInterface;
class Profile;
class SidePanelRegistry;
class SidePanelEntryScope;
class SidePanelWebUIView;
class WalletPageUI;

namespace sidebar {
class SidebarController;
}  // namespace sidebar

class WalletSidePanelCoordinator : public views::ViewObserver {
 public:
  WalletSidePanelCoordinator(BrowserWindowInterface* browser,
                             sidebar::SidebarController* sidebar_controller,
                             Profile* profile);
  WalletSidePanelCoordinator(const WalletSidePanelCoordinator&) = delete;
  WalletSidePanelCoordinator& operator=(const WalletSidePanelCoordinator&) =
      delete;
  ~WalletSidePanelCoordinator() override;

  void CreateAndRegisterEntry(SidePanelRegistry* global_registry);

  void ActivatePanel();

  SidePanelWebUIView* side_panel_web_view() { return side_panel_web_view_; }

  // views::ViewObserver:
  void OnViewIsDeleting(views::View* view) override;

 private:
  std::unique_ptr<views::View> CreateWebView(SidePanelEntryScope& scope);

  const raw_ptr<BrowserWindowInterface> browser_;
  const raw_ptr<sidebar::SidebarController> sidebar_controller_;
  const raw_ptr<Profile> profile_;
  raw_ptr<SidePanelRegistry> registry_ = nullptr;

  std::unique_ptr<WebUIContentsWrapperT<WalletPageUI>> contents_wrapper_;

  raw_ptr<SidePanelWebUIView> side_panel_web_view_ = nullptr;

  base::ScopedObservation<views::View, views::ViewObserver> view_observation_{
      this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_WALLET_WALLET_SIDE_PANEL_COORDINATOR_H_
