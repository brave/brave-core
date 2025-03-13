/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

#define BrowserWindowFeatures BrowserWindowFeatures_ChromiumImpl
#define InitPostBrowserViewConstruction virtual InitPostBrowserViewConstruction

#include "src/chrome/browser/ui/browser_window/public/browser_window_features.h"  // IWYU pragma: export

#undef InitPostBrowserViewConstruction
#undef BrowserWindowFeatures

class BraveVPNController;

class BrowserWindowFeatures : public BrowserWindowFeatures_ChromiumImpl {
 public:
  static std::unique_ptr<BrowserWindowFeatures> CreateBrowserWindowFeatures();
  ~BrowserWindowFeatures() override;

  // Call this method to stub out BrowserWindowFeatures for tests.
  using BrowserWindowFeaturesFactory =
      base::RepeatingCallback<std::unique_ptr<BrowserWindowFeatures>()>;
  static void ReplaceBrowserWindowFeaturesForTesting(
      BrowserWindowFeaturesFactory factory);

  // BrowserWindowFeatures_ChromiumImpl:
  void InitPostBrowserViewConstruction(BrowserView* browser_view) override;

  BraveVPNController* GetBraveVPNController();

 protected:
  BrowserWindowFeatures();

 private:
  std::unique_ptr<BraveVPNController> brave_vpn_controller_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
