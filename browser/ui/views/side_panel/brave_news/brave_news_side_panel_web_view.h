// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_NEWS_BRAVE_NEWS_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_NEWS_BRAVE_NEWS_SIDE_PANEL_WEB_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"

class BraveNewsUI;
class Profile;

class BraveNewsSidePanelWebView : public SidePanelWebUIViewT<BraveNewsUI> {
 public:
  static std::unique_ptr<views::View> CreateView(Profile* profile,
                                                 SidePanelEntryScope& scope);

  BraveNewsSidePanelWebView(
      SidePanelEntryScope& scope,
      std::unique_ptr<WebUIContentsWrapperT<BraveNewsUI>> contents_wrapper);
  BraveNewsSidePanelWebView(const BraveNewsSidePanelWebView&) = delete;
  BraveNewsSidePanelWebView& operator=(const BraveNewsSidePanelWebView&) =
      delete;
  ~BraveNewsSidePanelWebView() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_NEWS_BRAVE_NEWS_SIDE_PANEL_WEB_VIEW_H_
