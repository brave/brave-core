/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_

// To prevent widely used "Init" re-definition.
#include "base/functional/callback.h"
#include "chrome/common/buildflags.h"

class BrowserWindowFeatures;
using BraveBrowserWindowFeatures = BrowserWindowFeatures;

#define BrowserWindowFeatures BrowserWindowFeatures_ChromiumImpl
#define Init virtual Init
#define InitPostBrowserViewConstruction virtual InitPostBrowserViewConstruction
#define InitPostWindowConstruction virtual InitPostWindowConstruction
#define TearDownPreBrowserWindowDestruction(...)            \
  virtual TearDownPreBrowserWindowDestruction(__VA_ARGS__); \
  friend BraveBrowserWindowFeatures

// Add const version of exclusive_access_manager()
#define exclusive_access_manager()          \
  exclusive_access_manager() {              \
    return exclusive_access_manager_.get(); \
  }                                         \
  const ExclusiveAccessManager* exclusive_access_manager() const

// Add const version of side_panel_ui()
#define side_panel_ui() \
  side_panel_ui();      \
  const SidePanelUI* side_panel_ui() const

#include <chrome/browser/ui/browser_window/public/browser_window_features.h>  // IWYU pragma: export

#undef side_panel_ui
#undef exclusive_access_manager
#undef TearDownPreBrowserWindowDestruction
#undef InitPostWindowConstruction
#undef InitPostBrowserViewConstruction
#undef Init
#undef BrowserWindowFeatures

#include "brave/browser/ui/browser_window/public/browser_window_features.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
