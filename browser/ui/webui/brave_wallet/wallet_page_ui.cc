/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/meld_integration_service_factory.h"
#include "brave/browser/brave_wallet/swap_service_factory.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_handler.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
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
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/sanitized_image_source.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/webui/webui_util.h"

WalletPageUI::WalletPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui,
                              true /* Needed for webui browser tests */) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kWalletPageHost);
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str = l10n_util::GetStringUTF16(str.id);
    source->AddString(str.name, l10n_str);
  }
  auto plural_string_handler = std::make_unique<PluralStringHandler>();
  plural_string_handler->AddLocalizedString(
      "braveWalletExchangeNamePlusSteps",
      IDS_BRAVE_WALLET_EXCHANGE_NAME_PLUS_STEPS);
  plural_string_handler->AddLocalizedString(
      "braveWalletPendingTransactions", IDS_BRAVE_WALLET_PENDING_TRANSACTIONS);
  web_ui->AddMessageHandler(std::move(plural_string_handler));
  NavigationBarDataProvider::Initialize(source, profile);
  webui::SetupWebUIDataSource(source, base::span(kBraveWalletPageGenerated),
                              IDR_WALLET_PAGE_HTML);
  source->AddString("braveWalletLedgerBridgeUrl", kUntrustedLedgerURL);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' data: chrome://resources chrome://erc-token-images "
      "chrome://image;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kUntrustedTrezorURL + " " +
          kUntrustedLedgerURL + " " + kUntrustedNftURL + " " +
          kUntrustedLineChartURL + " " + kUntrustedMarketURL + " " +
          kUntrustedPolkadotURL + ";");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome://resources chrome-untrusted://resources 'self' "
      "'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src chrome://resources chrome-untrusted://resources 'self' "
      "'wasm-unsafe-eval';");
  source->AddString("braveWalletTrezorBridgeUrl", kUntrustedTrezorURL);
  source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  source->AddString("braveWalletPolkadotBridgeUrl", kUntrustedPolkadotURL);
  source->AddString("braveWalletLineChartBridgeUrl", kUntrustedLineChartURL);
  source->AddString("braveWalletMarketUiBridgeUrl", kUntrustedMarketURL);
  source->AddBoolean("isMobile", false);
  source->AddBoolean(brave_wallet::mojom::kP3ACountTestNetworksLoadTimeKey,
                     base::CommandLine::ForCurrentProcess()->HasSwitch(
                         brave_wallet::mojom::kP3ACountTestNetworksSwitch));
  source->AddBoolean("rewardsFeatureEnabled",
                     brave_rewards::IsSupportedForProfile(profile));
  content::URLDataSource::Add(profile,
                              std::make_unique<SanitizedImageSource>(profile));
  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(profile));
  brave_wallet::AddBlockchainTokenImageSource(profile);
}

WalletPageUI::~WalletPageUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(WalletPageUI)

void WalletPageUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void WalletPageUI::BindInterface(
    mojo::PendingReceiver<brave_rewards::mojom::RewardsPageHandler> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);

  rewards_handler_ = std::make_unique<brave_rewards::RewardsPageHandler>(
      std::move(receiver), nullptr,
      brave_rewards::RewardsServiceFactory::GetForProfile(profile),
      brave_ads::AdsServiceFactory::GetForProfile(profile), nullptr,
      profile->GetPrefs());
}

void WalletPageUI::CreatePageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> page_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService>
        json_rpc_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
        bitcoin_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::PolkadotWalletService>
        polkadot_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::ZCashWalletService>
        zcash_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::CardanoWalletService>
        cardano_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::SwapService>
        swap_service_receiver,
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
    mojo::PendingReceiver<brave_wallet::mojom::BtcTxManagerProxy>
        bitcoin_tx_manager_proxy_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
        brave_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A>
        brave_wallet_p3a_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
        ipfs_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::MeldIntegrationService>
        meld_integration_service) {
  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);

  page_handler_ =
      std::make_unique<WalletPageHandler>(std::move(page_receiver), profile);
  wallet_handler_ = std::make_unique<brave_wallet::WalletHandler>(
      std::move(wallet_receiver), profile);

  if (auto* wallet_service =
          brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
              profile)) {
    wallet_service->Bind(std::move(brave_wallet_service_receiver));
    wallet_service->Bind(std::move(json_rpc_service_receiver));
    wallet_service->Bind(std::move(bitcoin_wallet_service_receiver));
    wallet_service->Bind(std::move(polkadot_wallet_service_receiver));
    wallet_service->Bind(std::move(zcash_wallet_service_receiver));
    wallet_service->Bind(std::move(cardano_wallet_service_receiver));
    wallet_service->Bind(std::move(keyring_service_receiver));
    wallet_service->Bind(std::move(tx_service_receiver));
    wallet_service->Bind(std::move(eth_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(solana_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(filecoin_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(bitcoin_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(brave_wallet_p3a_receiver));
  }

  brave_wallet::SwapServiceFactory::BindForContext(
      profile, std::move(swap_service_receiver));
  brave_wallet::AssetRatioServiceFactory::BindForContext(
      profile, std::move(asset_ratio_service_receiver));
  brave_wallet::MeldIntegrationServiceFactory::BindForContext(
      profile, std::move(meld_integration_service));
  brave_wallet::BraveWalletIpfsServiceFactory::BindForContext(
      profile, std::move(ipfs_service_receiver));

  auto* blockchain_registry = brave_wallet::BlockchainRegistry::GetInstance();
  if (blockchain_registry) {
    blockchain_registry->Bind(std::move(blockchain_registry_receiver));
  }
}

WalletPageUIConfig::WalletPageUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kWalletPageHost) {}

bool WalletPageUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return brave_wallet::IsNativeWalletEnabled() &&
         brave_wallet::IsAllowedForContext(browser_context);
}
