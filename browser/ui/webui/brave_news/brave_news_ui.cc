// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news/brave_news_ui.h"

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

BraveNewsUI::BraveNewsUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, /*enable_chrome_send=*/true) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      Profile::FromWebUI(web_ui), kBraveNewsHost);
  webui::SetupWebUIDataSource(source, /*resources=*/{},
                              IDR_BRAVE_NEWS_PANEL_HTML);
}

BraveNewsUI::~BraveNewsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewsUI)

BraveNewsUIConfig::BraveNewsUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme, kBraveNewsHost) {}

bool BraveNewsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return true;
}
