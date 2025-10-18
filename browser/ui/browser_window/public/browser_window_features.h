/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

#include <memory>

class BraveVPNController;
class PlaylistSidePanelCoordinator;

namespace brave_rewards {
class RewardsPanelCoordinator;
}  // namespace brave_rewards

namespace sidebar {
class SidebarController;
}  // namespace sidebar

// This file doesn't include header file for BrowserWindowFeatures_ChromiumImpl
// because this file only could be included at the bottom of
// //chrome/browser/ui/browser_window/public/browser_window_features.h. So we
// could avoid dependency cycle with //chrome/browser/ui/browser_window.
class BrowserWindowFeatures : public BrowserWindowFeatures_ChromiumImpl {
 public:
  BrowserWindowFeatures();
  ~BrowserWindowFeatures() override;

  // BrowserWindowFeatures_ChromiumImpl:
  void Init(BrowserWindowInterface* browser) override;
  void InitPostBrowserViewConstruction(BrowserView* browser_view) override;
  void TearDownPreBrowserWindowDestruction() override;

  sidebar::SidebarController* sidebar_controller() {
    return sidebar_controller_.get();
  }
  brave_rewards::RewardsPanelCoordinator* rewards_panel_coordinator() {
    return rewards_panel_coordinator_.get();
  }
  BraveVPNController* brave_vpn_controller();

  PlaylistSidePanelCoordinator* playlist_side_panel_coordinator() {
    return playlist_side_panel_coordinator_.get();
  }

 private:
  std::unique_ptr<sidebar::SidebarController> sidebar_controller_;
  std::unique_ptr<BraveVPNController> brave_vpn_controller_;
  std::unique_ptr<brave_rewards::RewardsPanelCoordinator>
      rewards_panel_coordinator_;
  std::unique_ptr<PlaylistSidePanelCoordinator>
      playlist_side_panel_coordinator_;
};

#endif  // BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
