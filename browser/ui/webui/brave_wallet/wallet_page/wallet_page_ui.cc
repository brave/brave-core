/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_page/wallet_page_ui.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "brave/browser/brave_wallet/blockchain_images_source.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page/wallet_page_handler.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/sanitized_image/sanitized_image_source.h"
#include "components/favicon_base/favicon_url_parser.h"
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

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/brave_ads/ads_service_factory.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_handler.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

namespace {

bool IsMobile() {
#if BUILDFLAG(IS_ANDROID)
  return true;
#else
  return false;
#endif
}

bool IsRewardsFeatureEnabled(Profile* profile) {
  // Rewards UI features are not currently supported on Android.
#if BUILDFLAG(ENABLE_BRAVE_REWARDS) && !BUILDFLAG(IS_ANDROID)
  return brave_rewards::IsSupportedForProfile(profile);
#else
  return false;
#endif
}

}  // namespace

namespace brave_wallet {

WalletPageUI::WalletPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui,
                              true /* Needed for webui browser tests */) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kWalletPageHost);
  webui::SetupWebUIDataSource(source, base::span(kBraveWalletPageGenerated),
                              IDR_WALLET_PAGE_HTML);
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);

  for (const auto& str : kLocalizedStrings) {
    source->AddString(str.name, l10n_util::GetStringUTF16(str.id));
  }

#if !BUILDFLAG(IS_ANDROID)
  auto plural_string_handler = std::make_unique<PluralStringHandler>();
  plural_string_handler->AddLocalizedString(
      "braveWalletExchangeNamePlusSteps",
      IDS_BRAVE_WALLET_EXCHANGE_NAME_PLUS_STEPS);
  plural_string_handler->AddLocalizedString(
      "braveWalletPendingTransactions", IDS_BRAVE_WALLET_PENDING_TRANSACTIONS);
  plural_string_handler->AddLocalizedString(
      "braveWalletHardwareWalletAccountConnectedSuccessfully",
      IDS_BRAVE_WALLET_HARDWARE_WALLET_ACCOUNT_CONNECTED_SUCCESSFULLY);
  web_ui->AddMessageHandler(std::move(plural_string_handler));
#endif

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' data: chrome://resources chrome://erc-token-images "
      "chrome://image;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kUntrustedTrezorURL + " " +
          kUntrustedLedgerURL + " " + kUntrustedNftURL + " " +
          kUntrustedLineChartURL + " " + kUntrustedMarketURL + ";");
  source->AddString("braveWalletLedgerBridgeUrl", kUntrustedLedgerURL);
  source->AddString("braveWalletTrezorBridgeUrl", kUntrustedTrezorURL);
  source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  source->AddString("braveWalletLineChartBridgeUrl", kUntrustedLineChartURL);
  source->AddString("braveWalletMarketUiBridgeUrl", kUntrustedMarketURL);
  source->AddBoolean("isMobile", IsMobile());
  source->AddBoolean("isIOS", false);
  source->AddBoolean(mojom::kP3ACountTestNetworksLoadTimeKey,
                     base::CommandLine::ForCurrentProcess()->HasSwitch(
                         mojom::kP3ACountTestNetworksSwitch));
  source->AddBoolean("rewardsFeatureEnabled", IsRewardsFeatureEnabled(profile));
  source->AddBoolean("walletDebug", IsWalletDebugEnabled());

#if !BUILDFLAG(IS_ANDROID)
  content::URLDataSource::Add(profile, std::make_unique<ThemeSource>(profile));
#endif

  content::URLDataSource::Add(profile,
                              std::make_unique<SanitizedImageSource>(profile));
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
  content::URLDataSource::Add(
      profile, std::make_unique<BlockchainImagesSource>(profile));
}

WalletPageUI::~WalletPageUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(WalletPageUI)

void WalletPageUI::BindInterface(
    mojo::PendingReceiver<mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void WalletPageUI::BindInterface(
    mojo::PendingReceiver<brave_rewards::mojom::RewardsPageHandler> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);

  rewards_handler_ = std::make_unique<brave_rewards::RewardsPageHandler>(
      std::move(receiver), nullptr,
      brave_rewards::RewardsServiceFactory::GetForProfile(profile),
#if BUILDFLAG(ENABLE_BRAVE_ADS)
      brave_ads::AdsServiceFactory::GetForProfile(profile),
#else
      nullptr,
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
      nullptr, profile->GetPrefs());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

void WalletPageUI::CreatePageHandler(
    mojo::PendingReceiver<mojom::PageHandler> page_receiver,
    mojo::PendingReceiver<mojom::WalletHandler> wallet_receiver,
    mojo::PendingReceiver<mojom::JsonRpcService> json_rpc_service_receiver,
    mojo::PendingReceiver<mojom::BitcoinWalletService>
        bitcoin_wallet_service_receiver,
    mojo::PendingReceiver<mojom::PolkadotWalletService>
        polkadot_wallet_service_receiver,
    mojo::PendingReceiver<mojom::ZCashWalletService>
        zcash_wallet_service_receiver,
    mojo::PendingReceiver<mojom::CardanoWalletService>
        cardano_wallet_service_receiver,
    mojo::PendingReceiver<mojom::SwapService> swap_service_receiver,
    mojo::PendingReceiver<mojom::AssetRatioService>
        asset_ratio_service_receiver,
    mojo::PendingReceiver<mojom::KeyringService> keyring_service_receiver,
    mojo::PendingReceiver<mojom::BlockchainRegistry>
        blockchain_registry_receiver,
    mojo::PendingReceiver<mojom::TxService> tx_service_receiver,
    mojo::PendingReceiver<mojom::EthTxManagerProxy>
        eth_tx_manager_proxy_receiver,
    mojo::PendingReceiver<mojom::SolanaTxManagerProxy>
        solana_tx_manager_proxy_receiver,
    mojo::PendingReceiver<mojom::FilTxManagerProxy>
        filecoin_tx_manager_proxy_receiver,
    mojo::PendingReceiver<mojom::BtcTxManagerProxy>
        bitcoin_tx_manager_proxy_receiver,
    mojo::PendingReceiver<mojom::BraveWalletService>
        brave_wallet_service_receiver,
    mojo::PendingReceiver<mojom::BraveWalletP3A> brave_wallet_p3a_receiver,
    mojo::PendingReceiver<mojom::IpfsService> ipfs_service_receiver,
    mojo::PendingReceiver<mojom::MeldIntegrationService>
        meld_integration_service) {
  auto* profile = Profile::FromWebUI(web_ui());
  CHECK(profile);

  page_handler_ = std::make_unique<WalletPageHandler>(std::move(page_receiver),
                                                      profile, *this);

  if (auto* wallet_service =
          BraveWalletServiceFactory::GetServiceForContext(profile)) {
    wallet_handler_ = std::make_unique<WalletHandler>(
        std::move(wallet_receiver), wallet_service);
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
    wallet_service->Bind(std::move(asset_ratio_service_receiver));
    wallet_service->Bind(std::move(swap_service_receiver));
    wallet_service->Bind(std::move(meld_integration_service));
    wallet_service->Bind(std::move(ipfs_service_receiver));
  }

  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  if (blockchain_registry) {
    blockchain_registry->Bind(std::move(blockchain_registry_receiver));
  }
  WalletInteractionDetected(web_ui()->GetWebContents());
}

WalletPageUIConfig::WalletPageUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kWalletPageHost) {}

bool WalletPageUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  if (!IsAllowedForContext(browser_context)) {
    return false;
  }
#if BUILDFLAG(IS_ANDROID)
  auto* brave_wallet_service =
      BraveWalletServiceFactory::GetServiceForContext(browser_context);
  // Don't support wallet page webUI until wallet is setup with native Android
  // UI.
  if (!brave_wallet_service ||
      !brave_wallet_service->keyring_service()->IsWalletCreatedSync()) {
    return false;
  }
#endif
  return true;
}

}  // namespace brave_wallet
