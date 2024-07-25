/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_BOOKMARKS_BOOKMARKS_SIDE_PANEL_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_BOOKMARKS_BOOKMARKS_SIDE_PANEL_UI_H_

#define BookmarksSidePanelUI BookmarksSidePanelUI_ChromiumImpl
#define BookmarksSidePanelUIConfig BookmarksSidePanelUIConfig_Unused

#include "src/chrome/browser/ui/webui/side_panel/bookmarks/bookmarks_side_panel_ui.h"  // IWYU pragma: export

#undef BookmarksSidePanelUIConfig
#undef BookmarksSidePanelUI

class BookmarksSidePanelUI : public BookmarksSidePanelUI_ChromiumImpl {
 public:
  explicit BookmarksSidePanelUI(content::WebUI* web_ui);
  ~BookmarksSidePanelUI() override;
};

class BookmarksSidePanelUIConfig
    : public DefaultTopChromeWebUIConfig<BookmarksSidePanelUI> {
 public:
  BookmarksSidePanelUIConfig()
      : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                    chrome::kChromeUIBookmarksSidePanelHost) {}
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_BOOKMARKS_BOOKMARKS_SIDE_PANEL_UI_H_
