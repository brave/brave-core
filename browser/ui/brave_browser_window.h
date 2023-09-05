/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_

#include "brave/components/ai_chat/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_window.h"

namespace content {
class WebContents;
}  // namespace content

#if defined(TOOLKIT_VIEWS)
namespace sidebar {
class Sidebar;
}  // namespace sidebar
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
namespace speedreader {
class SpeedreaderBubbleView;
class SpeedreaderTabHelper;
enum class SpeedreaderBubbleLocation : int;
}  // namespace speedreader
#endif

class BraveBrowserWindow : public BrowserWindow {
 public:
  ~BraveBrowserWindow() override {}

  static BraveBrowserWindow* From(BrowserWindow*);

  virtual void StartTabCycling() {}

  // Returns the rectangle info of the Shield's panel.
  // Renderers will call this to check if the bottom of the panel exceeds
  // the overall screen's height
  virtual gfx::Rect GetShieldsBubbleRect();

#if BUILDFLAG(ENABLE_SPEEDREADER)
  virtual speedreader::SpeedreaderBubbleView* ShowSpeedreaderBubble(
      speedreader::SpeedreaderTabHelper* tab_helper,
      speedreader::SpeedreaderBubbleLocation location);
  virtual void ShowReaderModeToolbar() {}
  virtual void HideReaderModeToolbar() {}
#endif

#if defined(TOOLKIT_VIEWS)
  virtual sidebar::Sidebar* InitSidebar();
  virtual void ToggleSidebar();
  virtual bool HasSelectedURL() const;
  virtual void CleanAndCopySelectedURL();
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  virtual void ShowPlaylistBubble() {}
#endif

  virtual void ShowBraveVPNBubble() {}
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_WINDOW_H_
