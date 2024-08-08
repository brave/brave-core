/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/playlist/playlist_browser_finder.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace playlist {

namespace {

BrowserView* FindBrowserViewFromSidebarContents(
    content::WebContents* contents) {
  auto* proxy = PlaylistSidePanelCoordinator::Proxy::FromWebContents(contents);
  if (!proxy) {
    return nullptr;
  }

  auto coordinator = proxy->GetCoordinator();
  if (!coordinator) {
    return nullptr;
  }

  return coordinator->GetBrowserView();
}

}  // namespace

// Implementation for playlist_browser_finder.h
Browser* FindBrowserForPlaylistWebUI(content::WebContents* web_contents) {
  if (auto* browser_view = FindBrowserViewFromSidebarContents(web_contents)) {
    return browser_view->browser();
  }

  // If |BrowserView| is not found from Sidebar's |WebContents|, try to find it
  // from tab's |WebContents|.
  // https://github.com/brave/brave-browser/issues/37528
  return chrome::FindBrowserWithTab(web_contents);
}

}  // namespace playlist
