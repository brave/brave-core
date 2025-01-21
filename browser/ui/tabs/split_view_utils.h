/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_UTILS_H_
#define BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_UTILS_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "chrome/browser/ui/tabs/tab_model.h"

class Browser;

namespace content {
class WebContents;
}

namespace SplitViewUtils {

struct TileContents {
  TileContents() = default;
  ~TileContents() = default;

  raw_ptr<content::WebContents> main = nullptr;
  raw_ptr<content::WebContents> secondary = nullptr;
  bool is_main_active = false;
  bool is_secondary_active = false;
};

bool IsSplitViewEnabled();

tabs::TabHandle GetWebContentsTabHandle(Browser* browser,
                                        content::WebContents* web_contents);
tabs::TabHandle GetActiveWebContentsTabHandle(Browser* browser);

bool IsWebContentsTiled(Browser* browser, content::WebContents* web_contents);
bool IsActiveWebContentsTiled(Browser* browser);

content::WebContents* GetTabWebContents(Browser* browser,
                                        const tabs::TabHandle& handle);

std::optional<TabTile> GetWebContentsTile(Browser* browser,
                                          content::WebContents* web_contents);

TileContents GetTileContents(Browser* browser, const TabTile& tile);

TileContents GetTabTileContents(Browser* browser,
                                content::WebContents* web_contents);

TileContents GetActiveTabTileContents(Browser* browser);

}  // namespace SplitViewUtils

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_UTILS_H_
