/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_ui.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_page_handler.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/resources/cookie_list_opt_in/grit/cookie_list_opt_in_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

static constexpr webui::LocalizedString kStrings[] = {
    {"cookieListTitle", IDS_BRAVE_SHIELDS_COOKIE_LIST_TITLE},
    {"cookieListHeader", IDS_BRAVE_SHIELDS_COOKIE_LIST_HEADER},
    {"cookieListText", IDS_BRAVE_SHIELDS_COOKIE_LIST_TEXT},
    {"cookieListButtonText", IDS_BRAVE_SHIELDS_COOKIE_LIST_BUTTON_TEXT},
    {"cookieListNoThanks", IDS_BRAVE_SHIELDS_COOKIE_LIST_NO_THANKS}};

}  // namespace

CookieListOptInUI::CookieListOptInUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true) {
  DCHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kBraveAdblockCookieListOptIn));

  auto* profile = Profile::FromWebUI(web_ui);

  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kCookieListOptInHost);
  source->AddLocalizedStrings(kStrings);

  webui::SetupWebUIDataSource(source, kCookieListOptInGenerated,
                              IDR_COOKIE_LIST_OPT_IN_HTML);

  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

CookieListOptInUI::~CookieListOptInUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(CookieListOptInUI)

void CookieListOptInUI::BindInterface(
    mojo::PendingReceiver<CookieListOptInPageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void CookieListOptInUI::CreatePageHandler(
    mojo::PendingReceiver<brave_shields::mojom::CookieListOptInPageHandler>
        receiver) {
  page_handler_ = std::make_unique<CookieListOptInPageHandler>(
      std::move(receiver), embedder(), Profile::FromWebUI(web_ui()));
}

CookieListOptInUIConfig::CookieListOptInUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  kCookieListOptInHost) {}

bool CookieListOptInUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(
      brave_shields::features::kBraveAdblockCookieListOptIn);
}

bool CookieListOptInUIConfig::ShouldAutoResizeHost() {
  return true;
}
