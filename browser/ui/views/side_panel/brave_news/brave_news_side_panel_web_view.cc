// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_news/brave_news_side_panel_web_view.h"

#include "base/check.h"
#include "brave/browser/ui/webui/brave_news/brave_news_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "url/gurl.h"

using SidePanelWebUIViewT_BraveNewsUI = SidePanelWebUIViewT<BraveNewsUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_BraveNewsUI, SidePanelWebUIViewT)
END_METADATA

// static
std::unique_ptr<views::View> BraveNewsSidePanelWebView::CreateView(
    Profile* profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);
  auto web_view = std::make_unique<BraveNewsSidePanelWebView>(
      scope, std::make_unique<WebUIContentsWrapperT<BraveNewsUI>>(
                 GURL(kBraveNewsURL), profile, IDS_BRAVE_NEWS_TITLE,
                 /*esc_closes_ui=*/false));
  web_view->ShowUI();
  return web_view;
}

BraveNewsSidePanelWebView::BraveNewsSidePanelWebView(
    SidePanelEntryScope& scope,
    std::unique_ptr<WebUIContentsWrapperT<BraveNewsUI>> contents_wrapper)
    : SidePanelWebUIViewT<BraveNewsUI>(scope,
                                       base::RepeatingClosure(),
                                       base::RepeatingClosure(),
                                       std::move(contents_wrapper)) {}

BraveNewsSidePanelWebView::~BraveNewsSidePanelWebView() = default;
