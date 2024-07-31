/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/side_panel/bookmarks/bookmarks_side_panel_ui.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/webui/side_panel/bookmarks/bookmarks_page_handler.h"
#include "ui/base/l10n/l10n_util.h"

#define BookmarksSidePanelUIConfig BookmarksSidePanelUIConfig_Unused
#define BookmarksSidePanelUI BookmarksSidePanelUI_ChromiumImpl

#include "src/chrome/browser/ui/webui/side_panel/bookmarks/bookmarks_side_panel_ui.cc"

#undef BookmarksSidePanelUIConfig
#undef BookmarksSidePanelUI

BookmarksSidePanelUIConfig::BookmarksSidePanelUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  chrome::kChromeUIBookmarksSidePanelHost) {}

bool BookmarksSidePanelUIConfig::IsPreloadable() {
  return true;
}

std::optional<int> BookmarksSidePanelUIConfig::GetCommandIdForTesting() {
  return IDC_SHOW_BOOKMARK_SIDE_PANEL;
}

BookmarksSidePanelUI::BookmarksSidePanelUI(content::WebUI* web_ui)
    : BookmarksSidePanelUI_ChromiumImpl(web_ui) {
  base::Value::Dict update_data;
  update_data.Set("sortCustom",
                  l10n_util::GetStringUTF16(IDS_BOOKMARKS_SORT_CUSTOM));
  update_data.Set("sortCustomLower",
                  l10n_util::GetStringUTF16(IDS_BOOKMARKS_SORT_CUSTOM_LOWER));

  content::WebUIDataSource::Update(Profile::FromWebUI(web_ui),
                                   chrome::kChromeUIBookmarksSidePanelHost,
                                   std::move(update_data));
}

BookmarksSidePanelUI::~BookmarksSidePanelUI() = default;
