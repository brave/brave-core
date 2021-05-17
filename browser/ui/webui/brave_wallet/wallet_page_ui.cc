/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"

#include <utility>

#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_page_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/webui/web_ui_util.h"

WalletPageUI::WalletPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui,
                              true /* Needed for webui browser tests */) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kWalletPageHost);

  static constexpr webui::LocalizedString kStrings[] = {
      {"braveWallet", IDS_BRAVE_WALLET},
  };
  source->AddLocalizedStrings(kStrings);
  NavigationBarDataProvider::Initialize(source);
  webui::SetupWebUIDataSource(
      source,
      base::make_span(kBraveWalletPageGenerated, kBraveWalletPageGeneratedSize),
      IDR_WALLET_PAGE_HTML);
  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);
  Profile* profile = Profile::FromWebUI(web_ui);
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

WalletPageUI::~WalletPageUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(WalletPageUI)

void WalletPageUI::BindInterface(
    mojo::PendingReceiver<wallet_ui::mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void WalletPageUI::CreatePageHandler(
    mojo::PendingRemote<wallet_ui::mojom::Page> page,
    mojo::PendingReceiver<wallet_ui::mojom::PageHandler> page_receiver,
    mojo::PendingReceiver<wallet_ui::mojom::WalletHandler> wallet_receiver) {
  DCHECK(page);
  page_handler_ = std::make_unique<WalletPageHandler>(
      std::move(page_receiver), std::move(page), web_ui(), this);
  wallet_handler_ = std::make_unique<WalletHandler>(
      std::move(wallet_receiver), std::move(page), web_ui(), this);
}
