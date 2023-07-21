// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/android/android_wallet_page_ui.h"

#include <utility>

#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_deposit_page_generated_map.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_fund_wallet_page_generated_map.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_send_page_generated_map.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_swap_page_generated_map.h"
#include "brave/components/l10n/common/localization_util.h"

#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/browser/ui/webui/brave_webui_source.h"

#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/bitcoin_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/swap_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"

#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include "brave/components/constants/webui_url_constants.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"

#include "base/command_line.h"

AndroidWalletPageUI::AndroidWalletPageUI(content::WebUI* web_ui,
                                         const GURL& url)
    : ui::MojoWebUIController(web_ui,
                              true /* Needed for webui browser tests */) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kWalletPageHost);
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);

  for (const auto& str : brave_wallet::kLocalizedStrings) {
    std::u16string l10n_str =
        brave_l10n::GetLocalizedResourceUTF16String(str.id);
    source->AddString(str.name, l10n_str);
  }

  // Add required resources.
  if (url.path() == kWalletSwapPagePath) {
    webui::SetupWebUIDataSource(
        source,
        base::make_span(kBraveWalletSwapPageGenerated,
                        kBraveWalletSwapPageGeneratedSize),
        IDR_BRAVE_WALLET_SWAP_PAGE_HTML);
  } else if (url.path() == kWalletSendPagePath) {
    webui::SetupWebUIDataSource(
        source,
        base::make_span(kBraveWalletSendPageGenerated,
                        kBraveWalletSendPageGeneratedSize),
        IDR_BRAVE_WALLET_SEND_PAGE_HTML);

  } else if (url.path() == kWalletBuyPagePath) {
    webui::SetupWebUIDataSource(
        source,
        base::make_span(kBraveWalletFundWalletPageGenerated,
                        kBraveWalletFundWalletPageGeneratedSize),
        IDR_BRAVE_WALLET_FUND_WALLET_PAGE_HTML);

  } else if (url.path() == kWalletDepositPagePath) {
    webui::SetupWebUIDataSource(
        source,
        base::make_span(kBraveWalletDepositPageGenerated,
                        kBraveWalletDepositPageGeneratedSize),
        IDR_BRAVE_WALLET_DEPOSIT_PAGE_HTML);
  } else {
    NOTREACHED() << "Failed to find page resources for:" << url.path();
  }

  source->AddString("braveWalletLedgerBridgeUrl", kUntrustedLedgerURL);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kUntrustedTrezorURL + " " +
          kUntrustedLedgerURL + " " + kUntrustedNftURL + " " +
          kUntrustedMarketURL + ";");
  source->AddString("braveWalletTrezorBridgeUrl", kUntrustedTrezorURL);
  source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  source->AddString("braveWalletMarketUiBridgeUrl", kUntrustedMarketURL);
  source->AddBoolean(brave_wallet::mojom::kP3ACountTestNetworksLoadTimeKey,
                     base::CommandLine::ForCurrentProcess()->HasSwitch(
                         brave_wallet::mojom::kP3ACountTestNetworksSwitch));

  brave_wallet::AddBlockchainTokenImageSource(profile);
}

AndroidWalletPageUI::~AndroidWalletPageUI() = default;

void AndroidWalletPageUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(AndroidWalletPageUI)

void AndroidWalletPageUI::CreatePageHandler(
    mojo::PendingRemote<brave_wallet::mojom::Page> page,
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> page_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService>
        json_rpc_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
        bitcoin_rpc_service_receiver,
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
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
        brave_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A>
        brave_wallet_p3a_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletPinService>
        brave_wallet_pin_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletAutoPinService>
        brave_wallet_auto_pin_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
        ipfs_service_receiver) {
  DCHECK(page);
  auto* profile = Profile::FromWebUI(web_ui());
  DCHECK(profile);
  page_handler_ = std::make_unique<AndroidWalletPageHandler>(
      std::move(page_receiver), profile, this);
  wallet_handler_ = std::make_unique<brave_wallet::WalletHandler>(
      std::move(wallet_receiver), profile);

  brave_wallet::JsonRpcServiceFactory::BindForContext(
      profile, std::move(json_rpc_service_receiver));
  brave_wallet::BitcoinWalletServiceFactory::BindForContext(
      profile, std::move(bitcoin_rpc_service_receiver));
  brave_wallet::SwapServiceFactory::BindForContext(
      profile, std::move(swap_service_receiver));
  brave_wallet::AssetRatioServiceFactory::BindForContext(
      profile, std::move(asset_ratio_service_receiver));
  brave_wallet::KeyringServiceFactory::BindForContext(
      profile, std::move(keyring_service_receiver));
  brave_wallet::TxServiceFactory::BindForContext(
      profile, std::move(tx_service_receiver));
  brave_wallet::TxServiceFactory::BindEthTxManagerProxyForContext(
      profile, std::move(eth_tx_manager_proxy_receiver));
  brave_wallet::TxServiceFactory::BindSolanaTxManagerProxyForContext(
      profile, std::move(solana_tx_manager_proxy_receiver));
  brave_wallet::TxServiceFactory::BindFilTxManagerProxyForContext(
      profile, std::move(filecoin_tx_manager_proxy_receiver));
  brave_wallet::BraveWalletIpfsServiceFactory::BindForContext(
      profile, std::move(ipfs_service_receiver));
  brave_wallet::BraveWalletService* wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
  wallet_service->Bind(std::move(brave_wallet_service_receiver));
  wallet_service->GetBraveWalletP3A()->Bind(
      std::move(brave_wallet_p3a_receiver));

  auto* blockchain_registry = brave_wallet::BlockchainRegistry::GetInstance();
  if (blockchain_registry) {
    blockchain_registry->Bind(std::move(blockchain_registry_receiver));
  }
  brave_wallet::WalletInteractionDetected(web_ui()->GetWebContents());
}
