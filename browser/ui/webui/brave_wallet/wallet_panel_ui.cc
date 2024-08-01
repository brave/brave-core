/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/meld_integration_service_factory.h"
#include "brave/browser/brave_wallet/simulation_service_factory.h"
#include "brave/browser/brave_wallet/swap_service_factory.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_ipfs_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/simulation_service.h"
#include "brave/components/brave_wallet/browser/swap_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet_panel/resources/grit/brave_wallet_panel_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/sanitized_image_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/webui/web_ui_util.h"

WalletPanelUI::WalletPanelUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui,
                               true /* Needed for webui browser tests */) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kWalletPanelHost);
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }
  auto plural_string_handler = std::make_unique<PluralStringHandler>();
  plural_string_handler->AddLocalizedString(
      "braveWalletExchangeNamePlusSteps",
      IDS_BRAVE_WALLET_EXCHANGE_NAME_PLUS_STEPS);
  web_ui->AddMessageHandler(std::move(plural_string_handler));
  webui::SetupWebUIDataSource(source,
                              base::make_span(kBraveWalletPanelGenerated,
                                              kBraveWalletPanelGeneratedSize),
                              IDR_WALLET_PANEL_HTML);
  source->AddString("braveWalletLedgerBridgeUrl", kUntrustedLedgerURL);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::JoinString({"frame-src", kUntrustedTrezorURL, kUntrustedLedgerURL,
                        kUntrustedLineChartURL, kUntrustedNftURL,
                        base::StrCat({kUntrustedMarketURL, ";"})},
                       " "));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      base::JoinString(
          {"img-src", "'self'", "chrome://resources",
           "chrome://erc-token-images", "chrome://favicon", "chrome://image",
           "https://assets.cgproxy.brave.com", base::StrCat({"data:", ";"})},
          " "));
  source->AddString("braveWalletTrezorBridgeUrl", kUntrustedTrezorURL);
  source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  source->AddString("braveWalletLineChartBridgeUrl", kUntrustedLineChartURL);
  source->AddString("braveWalletMarketUiBridgeUrl", kUntrustedMarketURL);
  source->AddBoolean("isAndroid", false);
  source->AddBoolean(brave_wallet::mojom::kP3ACountTestNetworksLoadTimeKey,
                     base::CommandLine::ForCurrentProcess()->HasSwitch(
                         brave_wallet::mojom::kP3ACountTestNetworksSwitch));
  content::URLDataSource::Add(profile,
                              std::make_unique<SanitizedImageSource>(profile));
  brave_wallet::AddBlockchainTokenImageSource(profile);
  active_web_contents_ = brave_wallet::GetActiveWebContents();
}

WalletPanelUI::~WalletPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(WalletPanelUI)

void WalletPanelUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandlerFactory> receiver) {
  panel_factory_receiver_.reset();
  panel_factory_receiver_.Bind(std::move(receiver));
}

void WalletPanelUI::SetDeactivationCallback(
    base::RepeatingCallback<void(bool)> deactivation_callback) {
  deactivation_callback_ = std::move(deactivation_callback);
}

void WalletPanelUI::CreatePanelHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PanelHandler> panel_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService>
        json_rpc_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
        bitcoin_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::ZCashWalletService>
        zcash_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::SwapService>
        swap_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::SimulationService>
        simulation_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::AssetRatioService>
        asset_ratio_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::KeyringService>
        keyring_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BlockchainRegistry>
        blockchain_registry_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::TxService> tx_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::EthTxManagerProxy>
        eth_tx_manager_proxy_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::SolanaTxManagerProxy>
        solana_tx_manager_proxy_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::FilTxManagerProxy>
        filecoin_tx_manager_proxy_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
        brave_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A>
        brave_wallet_p3a_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
        brave_wallet_ipfs_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::MeldIntegrationService>
        meld_integration_service) {
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);

  panel_handler_ = std::make_unique<WalletPanelHandler>(
      std::move(panel_receiver), this, active_web_contents_,
      std::move(deactivation_callback_));
  wallet_handler_ = std::make_unique<brave_wallet::WalletHandler>(
      std::move(wallet_receiver), profile);

  if (auto* wallet_service =
          brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
              profile)) {
    wallet_service->Bind(std::move(brave_wallet_service_receiver));
    wallet_service->Bind(std::move(json_rpc_service_receiver));
    wallet_service->Bind(std::move(bitcoin_wallet_service_receiver));
    wallet_service->Bind(std::move(zcash_wallet_service_receiver));
    wallet_service->Bind(std::move(keyring_service_receiver));
    wallet_service->Bind(std::move(tx_service_receiver));
    wallet_service->Bind(std::move(eth_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(solana_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(filecoin_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(brave_wallet_p3a_receiver));
  }

  brave_wallet::SwapServiceFactory::BindForContext(
      profile, std::move(swap_service_receiver));
  brave_wallet::SimulationServiceFactory::BindForContext(
      profile, std::move(simulation_service_receiver));
  brave_wallet::AssetRatioServiceFactory::BindForContext(
      profile, std::move(asset_ratio_service_receiver));
  brave_wallet::MeldIntegrationServiceFactory::BindForContext(
      profile, std::move(meld_integration_service));
  brave_wallet::BraveWalletIpfsServiceFactory::BindForContext(
      profile, std::move(brave_wallet_ipfs_service_receiver));

  auto* blockchain_registry = brave_wallet::BlockchainRegistry::GetInstance();
  if (blockchain_registry) {
    blockchain_registry->Bind(std::move(blockchain_registry_receiver));
  }
}

WalletPanelUIConfig::WalletPanelUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme, kWalletPanelHost) {}

bool WalletPanelUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return brave_wallet::IsAllowedForContext(browser_context);
}

bool WalletPanelUIConfig::ShouldAutoResizeHost() {
  return true;
}
