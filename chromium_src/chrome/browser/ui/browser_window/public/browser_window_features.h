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

#include <chrome/browser/ui/browser_window/public/browser_window_features.h>  // IWYU pragma: export

#undef TearDownPreBrowserWindowDestruction
#undef InitPostWindowConstruction
#undef InitPostBrowserViewConstruction
#undef Init
#undef BrowserWindowFeatures

#include "brave/browser/ui/browser_window/public/browser_window_features.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_PUBLIC_BROWSER_WINDOW_FEATURES_H_
