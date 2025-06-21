/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_UI_H_

#define TabSearchUI TabSearchUI_ChromiumImpl
#define TabSearchUIConfig TabSearchUIConfig_Unused

#include <chrome/browser/ui/webui/tab_search/tab_search_ui.h>  // IWYU pragma: export

#undef TabSearchUI
#undef TabSearchUIConfig

class TabSearchUI : public TabSearchUI_ChromiumImpl {
 public:
  // To update WebUIDataSource for TabSearchUI with Brave specific resources.
  // Also override tab organization enabled boolean value with our own feature
  // flags.
  explicit TabSearchUI(content::WebUI* web_ui);
  ~TabSearchUI() override;
};

// Re-define TabSearchUIConfig so it will be using TabSearchUI instead of
// TabSearchUI_ChromiumImpl.
class TabSearchUIConfig : public DefaultTopChromeWebUIConfig<TabSearchUI> {
 public:
  TabSearchUIConfig();

  // DefaultTopChromeWebUIConfig:
  bool ShouldAutoResizeHost() override;
  bool IsPreloadable() override;
  std::optional<int> GetCommandIdForTesting() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_TAB_SEARCH_TAB_SEARCH_UI_H_
