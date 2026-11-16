/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_

#include "chrome/browser/ui/browser_window.h"

namespace content {
class WebContents;
}  // namespace content

class BraveBrowserWindow : public BrowserWindow {
 public:
  ~BraveBrowserWindow() override {}

  // Returns `window` as a BraveBrowserWindow, or nullptr when it is not a
  // BrowserView (e.g. a WebUIBrowserWindow or a TestBrowserWindow).
  static BraveBrowserWindow* From(BrowserWindow* window);
  static BraveBrowserWindow* FromBrowser(BrowserWindowInterface* browser);

  virtual void ReadyToListenFullscreenChanges() {}
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_
