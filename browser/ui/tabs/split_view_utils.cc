/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_utils.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/browser.h"

namespace SplitViewUtils {

bool IsSplitViewEnabled() {
  return base::FeatureList::IsEnabled(tabs::features::kBraveSplitView);
}

tabs::TabHandle GetWebContentsTabHandle(Browser* browser,
                                        content::WebContents* web_contents) {
  if (!browser || !web_contents) {
    return tabs::TabHandle::Null();
  }
  auto index = browser->tab_strip_model()->GetIndexOfWebContents(web_contents);
  if (index == TabStripModel::kNoTab) {
    return tabs::TabHandle::Null();
  }
  return browser->tab_strip_model()->GetTabHandleAt(index);
}

tabs::TabHandle GetActiveWebContentsTabHandle(Browser* browser) {
  if (!browser) {
    return tabs::TabHandle::Null();
  }
  return GetWebContentsTabHandle(
      browser, browser->tab_strip_model()->GetActiveWebContents());
}

bool IsWebContentsTiled(Browser* browser, content::WebContents* web_contents) {
  if (!IsSplitViewEnabled() || !web_contents || !browser ||
      !browser->is_type_normal()) {
    return false;
  }

  auto* split_view_browser_data = SplitViewBrowserData::FromBrowser(browser);
  return split_view_browser_data->IsTabTiled(
      GetWebContentsTabHandle(browser, web_contents));
}

bool IsActiveWebContentsTiled(Browser* browser) {
  return IsWebContentsTiled(browser,
                            browser->tab_strip_model()->GetActiveWebContents());
}

content::WebContents* GetTabWebContents(const tabs::TabHandle& handle) {
  if (!handle.Get()) {
    return nullptr;
  }
  return handle.Get()->contents();
}

std::optional<SplitViewBrowserData::Tile> GetWebContentsTile(
    Browser* browser,
    content::WebContents* web_contents) {
  if (!IsSplitViewEnabled()) {
    return std::nullopt;
  }

  auto* split_view_data = SplitViewBrowserData::FromBrowser(browser);
  auto handle = GetWebContentsTabHandle(browser, web_contents);
  return split_view_data->GetTile(handle);
}

TileContents GetTileContents(Browser* browser,
                             const SplitViewBrowserData::Tile& tile) {
  CHECK(IsSplitViewEnabled());

  TileContents result;
  if (!browser && browser->tab_strip_model()) {
    return result;
  }

  result.main = GetTabWebContents(tile.first);
  result.secondary = GetTabWebContents(tile.second);

  auto* active_tab = browser->tab_strip_model()->GetActiveTab();
  if (active_tab) {
    result.is_main_active = tile.first == active_tab->GetHandle();
    result.is_secondary_active = tile.second == active_tab->GetHandle();
  }

  return result;
}

TileContents GetTabTileContents(Browser* browser,
                                content::WebContents* web_contents) {
  TileContents result;
  if (!browser) {
    return result;
  }

  auto tab_handle = GetWebContentsTabHandle(browser, web_contents);
  if (!tab_handle.Get()) {
    return result;
  }

  if (IsSplitViewEnabled()) {
    auto* split_view_data = SplitViewBrowserData::FromBrowser(browser);
    auto tile = split_view_data->GetTile(tab_handle);
    if (!tile) {
      result.main = tab_handle.Get()->contents();
      result.is_main_active = true;
    } else {
      return GetTileContents(browser, tile.value());
    }
  } else {
    result.main = tab_handle.Get()->contents();
    result.is_main_active = true;
  }
  return result;
}

TileContents GetActiveTabTileContents(Browser* browser) {
  if (browser && browser->tab_strip_model()) {
    return GetTabTileContents(
        browser, browser->tab_strip_model()->GetActiveWebContents());
  }
  return {};
}

}  // namespace SplitViewUtils
