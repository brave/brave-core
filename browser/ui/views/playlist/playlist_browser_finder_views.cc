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

// If |BrowserView| is not found from Sidebar's |WebContents|, try to find it
// from tab's |WebContents|.
BrowserView* FindBrowserViewFromWebContents(content::WebContents* contents) {
  auto* browser_view = FindBrowserViewFromSidebarContents(contents);

  if (!browser_view) {
    auto* browser = chrome::FindBrowserWithTab(contents);
    return browser ? BrowserView::GetBrowserViewForBrowser(browser) : nullptr;
  }
  return browser_view;
}

}  // namespace

// Implementation for playlist_browser_finder.h
Browser* FindBrowserForPlaylistWebUI(content::WebContents* web_contents) {
  auto* browser_view = FindBrowserViewFromWebContents(web_contents);
  return browser_view ? browser_view->browser() : nullptr;
}

}  // namespace playlist
