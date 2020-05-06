// /* Copyright (c) 2019 The Brave Authors. All rights reserved.
//  * This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
//  * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_

#include "chrome/browser/ui/browser_window.h"

class BraveBrowserWindow : public BrowserWindow {
 public:
  ~BraveBrowserWindow() override {}

  virtual void StartMRUCycling() = 0;
};

#define BrowserWindow BraveBrowserWindow
#include "../../../../../../../chrome/browser/ui/views/frame/browser_view.h"  // NOLINT
#undef BrowserWindow

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_VIEW_H_
