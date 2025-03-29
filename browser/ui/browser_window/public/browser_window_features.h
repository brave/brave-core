/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

#include <memory>

class BraveVPNController;
class MobileViewSidePanelManager;
class SplitViewBrowserData;

// This file doesn't include header file for BrowserWindowFeatures_ChromiumImpl
// because this file only could be included at the bottom of
// //chrome/browser/ui/browser_window/public/browser_window_features.h. So we
// could avoid dependency cycle with //chrome/browser/ui/browser_window.
class BrowserWindowFeatures : public BrowserWindowFeatures_ChromiumImpl {
 public:
  static std::unique_ptr<BrowserWindowFeatures> CreateBrowserWindowFeatures();
  ~BrowserWindowFeatures() override;

  static void ReplaceBrowserWindowFeaturesForTesting(
      BrowserWindowFeaturesFactory factory);

  // BrowserWindowFeatures_ChromiumImpl:
  void Init(BrowserWindowInterface* browser_window_interface) override;
  void InitPostBrowserViewConstruction(BrowserView* browser_view) override;
  void TearDownPreBrowserViewDestruction() override;

  BraveVPNController* brave_vpn_controller();
  SplitViewBrowserData* split_view_browser_data() {
    return split_view_browser_data_.get();
  }
  MobileViewSidePanelManager* mobile_view_side_panel_manager() {
    return mobile_view_side_panel_manager_.get();
  }

 protected:
  BrowserWindowFeatures();

 private:
  std::unique_ptr<BraveVPNController> brave_vpn_controller_;
  std::unique_ptr<SplitViewBrowserData> split_view_browser_data_;
  std::unique_ptr<MobileViewSidePanelManager> mobile_view_side_panel_manager_;
};

#endif  // BRAVE_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
