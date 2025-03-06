// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_ui.h"

#include <utility>

#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/resources/brave_new_tab_page_refresh/grit/brave_new_tab_page_refresh_generated_map.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_refresh_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/common/webui_url_constants.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

BraveNewTabPageUI::BraveNewTabPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);

  auto* source = content::WebUIDataSource::CreateAndAdd(
      profile, chrome::kChromeUINewTabHost);

  if (brave::ShouldNewTabShowBlankpage(profile)) {
    source->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
  } else {
    webui::SetupWebUIDataSource(source, kBraveNewTabPageRefreshGenerated,
                                IDR_BRAVE_NEW_TAB_PAGE_HTML);
  }

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc, "img-src chrome://favicon2;");

  AddBackgroundColorToSource(source, web_ui->GetWebContents());

  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));

  web_ui->OverrideTitle(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_NEW_TAB_TITLE));
}

BraveNewTabPageUI::~BraveNewTabPageUI() = default;

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
        receiver) {
  page_handler_ =
      std::make_unique<BraveNewTabPageRefreshHandler>(std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabPageUI)
