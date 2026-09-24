/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_WEB_UI_VIEW_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_WEB_UI_VIEW_UTILS_H_

class GURL;

namespace brave {

// Helper function to determine if context menus should be enabled for the given
// URL Returns true for AI Chat URLs to enable features like spell check,
// copy/paste, etc.
bool ShouldEnableContextMenu(const GURL& url);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_WEB_UI_VIEW_UTILS_H_
