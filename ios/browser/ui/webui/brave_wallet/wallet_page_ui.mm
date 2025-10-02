// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_ui.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/command_line.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_ipfs_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/meld_integration_service.h"
#include "brave/components/brave_wallet/browser/simulation_service.h"
#include "brave/components/brave_wallet/browser/swap_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet_page/resources/grit/brave_wallet_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/meld_integration_service_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"

WalletPageUI::WalletPageUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()) {
  // Create a URLDataSource and add resources.

  BraveWebUIIOSDataSource* source = brave::CreateAndAddWebUIDataSource(
      web_ui, url.host(), kBraveWalletPageGenerated, IDR_WALLET_PAGE_HTML);

  source->AddLocalizedStrings(brave_wallet::kLocalizedStrings);

  source->AddBoolean("isMobile", true);
  source->AddString("braveWalletLedgerBridgeUrl", kUntrustedLedgerURL);
  source->AddString("braveWalletTrezorBridgeUrl", kUntrustedTrezorURL);
  source->AddString("braveWalletNftBridgeUrl", kUntrustedNftURL);
  source->AddString("braveWalletLineChartBridgeUrl", kUntrustedLineChartURL);
  source->AddString("braveWalletMarketUiBridgeUrl", kUntrustedMarketURL);
  source->AddBoolean(brave_wallet::mojom::kP3ACountTestNetworksLoadTimeKey,
                     base::CommandLine::ForCurrentProcess()->HasSwitch(
                         brave_wallet::mojom::kP3ACountTestNetworksSwitch));

  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&WalletPageUI::BindInterface,
                          base::Unretained(this)));
}

WalletPageUI::~WalletPageUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      "brave_wallet.mojom.PageHandlerFactory");
}

void WalletPageUI::BindInterface(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandlerFactory> receiver) {
  page_factory_receiver_.reset();
  page_factory_receiver_.Bind(std::move(receiver));
}

void WalletPageUI::CreatePageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> page_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService> json_rpc_service,
    mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
        bitcoin_rpc_service,
    mojo::PendingReceiver<brave_wallet::mojom::PolkadotWalletService>
        polkadot_wallet_service,
    mojo::PendingReceiver<brave_wallet::mojom::ZCashWalletService>
        zcash_service,
    mojo::PendingReceiver<brave_wallet::mojom::CardanoWalletService>
        cardano_wallet_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::SwapService> swap_service,
    mojo::PendingReceiver<brave_wallet::mojom::AssetRatioService>
        asset_ratio_service,
    mojo::PendingReceiver<brave_wallet::mojom::KeyringService> keyring_service,
    mojo::PendingReceiver<brave_wallet::mojom::BlockchainRegistry>
        blockchain_registry_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::TxService> tx_service,
    mojo::PendingReceiver<brave_wallet::mojom::EthTxManagerProxy>
        eth_tx_manager_proxy,
    mojo::PendingReceiver<brave_wallet::mojom::SolanaTxManagerProxy>
        solana_tx_manager_proxy,
    mojo::PendingReceiver<brave_wallet::mojom::FilTxManagerProxy>
        filecoin_tx_manager_proxy,
    mojo::PendingReceiver<brave_wallet::mojom::BtcTxManagerProxy>
        bitcoin_tx_manager_proxy_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletService>
        brave_wallet_service,
    mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A> brave_wallet_p3a,
    mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
        brave_wallet_ipfs_service_receiver,
    mojo::PendingReceiver<brave_wallet::mojom::MeldIntegrationService>
        meld_integration_service) {
  auto* profile = ProfileIOS::FromWebUIIOS(web_ui());
  DCHECK(profile);

  page_handler_ = std::make_unique<WalletPageHandler>(web_ui()->GetWebState(),
                                                      std::move(page_receiver));

  wallet_handler_ = std::make_unique<brave_wallet::WalletHandler>(
      std::move(wallet_receiver), profile);

  if (auto* wallet_service =
          brave_wallet::BraveWalletServiceFactory::GetServiceForState(
              profile)) {
    wallet_service->Bind(std::move(brave_wallet_service));
    wallet_service->Bind(std::move(json_rpc_service));
    wallet_service->Bind(std::move(bitcoin_rpc_service));
    wallet_service->Bind(std::move(polkadot_wallet_service));
    wallet_service->Bind(std::move(zcash_service));
    wallet_service->Bind(std::move(cardano_wallet_service_receiver));
    wallet_service->Bind(std::move(keyring_service));
    wallet_service->Bind(std::move(tx_service));
    wallet_service->Bind(std::move(eth_tx_manager_proxy));
    wallet_service->Bind(std::move(solana_tx_manager_proxy));
    wallet_service->Bind(std::move(filecoin_tx_manager_proxy));
    wallet_service->Bind(std::move(bitcoin_tx_manager_proxy_receiver));
    wallet_service->Bind(std::move(brave_wallet_p3a));
  }

  brave_wallet::SwapServiceFactory::GetServiceForState(profile)->Bind(
      std::move(swap_service));
  brave_wallet::AssetRatioServiceFactory::GetServiceForState(profile)->Bind(
      std::move(asset_ratio_service));
  brave_wallet::MeldIntegrationServiceFactory::GetServiceForState(profile)
      ->Bind(std::move(meld_integration_service));
  brave_wallet::BraveWalletIpfsServiceFactory::GetServiceForState(profile)
      ->Bind(std::move(brave_wallet_ipfs_service_receiver));

  if (auto* blockchain_registry =
          brave_wallet::BlockchainRegistry::GetInstance()) {
    blockchain_registry->Bind(std::move(blockchain_registry_receiver));
  }
}
