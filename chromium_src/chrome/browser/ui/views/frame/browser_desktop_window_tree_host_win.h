/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_DESKTOP_WINDOW_TREE_HOST_WIN_H_

class BrowserDesktopWindowTreeHostWin;
using BrowserDesktopWindowTreeHostWin_BraveImpl =
    BrowserDesktopWindowTreeHostWin;

#define BrowserDesktopWindowTreeHostWin \
  BrowserDesktopWindowTreeHostWin_ChromiumImpl

#define UpdateWorkspace() \
  UpdateWorkspace();      \
  friend BrowserDesktopWindowTreeHostWin_BraveImpl

#include "src/chrome/browser/ui/views/frame/browser_desktop_window_tree_host_win.h"  // IWYU pragma: export

#undef BrowserDesktopWindowTreeHostWin
#undef UpdateWorkspace

class BrowserDesktopWindowTreeHostWin
    : public BrowserDesktopWindowTreeHostWin_ChromiumImpl {
 public:
  using BrowserDesktopWindowTreeHostWin_ChromiumImpl::
      BrowserDesktopWindowTreeHostWin_ChromiumImpl;

 private:
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result) override;

  // Returns the optionally modified background color to correctly match the
  // toolbar color in dark/private browsing modes.
  SkColor GetBackgroundColor(SkColor requested_color) const override;

  SkColor GetToolbarColor() const;

  bool is_cloacked_ = false;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_DESKTOP_WINDOW_TREE_HOST_WIN_H_
