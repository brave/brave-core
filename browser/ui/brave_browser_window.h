/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_

#include "brave/components/sidebar/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_window.h"

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {
class Sidebar;
}  // namespace sidebar

namespace speedreader {
class SpeedreaderBubbleView;
class SpeedreaderTabHelper;
}  // namespace speedreader

class BraveBrowserWindow : public BrowserWindow {
 public:
  ~BraveBrowserWindow() override {}

  virtual void StartTabCycling() = 0;

  virtual speedreader::SpeedreaderBubbleView* ShowSpeedreaderBubble(
      speedreader::SpeedreaderTabHelper* tab_helper,
      bool is_enabled) = 0;

#if BUILDFLAG(ENABLE_SIDEBAR)
  virtual sidebar::Sidebar* InitSidebar() = 0;
#endif
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_
