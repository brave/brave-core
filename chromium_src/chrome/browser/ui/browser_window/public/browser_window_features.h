/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

class BraveVPNController;

#define InitPostBrowserViewConstruction                                    \
  InitPostBrowserViewConstruction_ChromiumImpl(BrowserView* browser_view); \
  BraveVPNController* GetBraveVPNController();                             \
                                                                           \
 private:                                                                  \
  std::unique_ptr<BraveVPNController> brave_vpn_controller_;               \
                                                                           \
 public:                                                                   \
  void InitPostBrowserViewConstruction

#include "src/chrome/browser/ui/browser_window/public/browser_window_features.h"  // IWYU pragma: export

#undef InitPostBrowserViewConstruction

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
