/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UI_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UI_H_

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"
#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_page_handler.h"
#include "brave/components/brave_rewards/core/mojom/rewards_page.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace brave_wallet {

class UntrustedPolkadotUI : public ui::MojoWebUIController,
                            public brave_wallet::mojom::PageHandlerFactory {
 public:
  explicit UntrustedPolkadotUI(content::WebUI* web_ui);
  UntrustedPolkadotUI(const UntrustedPolkadotUI&) = delete;
  UntrustedPolkadotUI& operator=(const UntrustedPolkadotUI&) = delete;
  ~UntrustedPolkadotUI() override;

  void BindInterface(
      mojo::PendingReceiver<brave_wallet::mojom::PageHandlerFactory> receiver);

 private:
  void CreatePageHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PageHandler> page_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> wallet_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::JsonRpcService>
          json_rpc_service,
      mojo::PendingReceiver<brave_wallet::mojom::BitcoinWalletService>
          bitcoin_rpc_service,
      mojo::PendingReceiver<brave_wallet::mojom::PolkadotWalletService>
          polkadot_rpc_service,
      mojo::PendingReceiver<brave_wallet::mojom::ZCashWalletService>
          zcash_service,
      mojo::PendingReceiver<brave_wallet::mojom::CardanoWalletService>
          cardano_wallet_service_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::SwapService> swap_service,
      mojo::PendingReceiver<brave_wallet::mojom::AssetRatioService>
          asset_ratio_service,
      mojo::PendingReceiver<brave_wallet::mojom::KeyringService>
          keyring_service,
      mojo::PendingReceiver<brave_wallet::mojom::BlockchainRegistry>
          blockchain_registry,
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
      mojo::PendingReceiver<brave_wallet::mojom::BraveWalletP3A>
          brave_wallet_p3a,
      mojo::PendingReceiver<brave_wallet::mojom::IpfsService>
          brave_wallet_ipfs_service_receiver,
      mojo::PendingReceiver<brave_wallet::mojom::MeldIntegrationService>
          meld_integration_service) override;

  std::unique_ptr<WalletPageHandler> page_handler_;
  mojo::Receiver<brave_wallet::mojom::PageHandlerFactory>
      page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedPolkadotUIConfig : public content::WebUIConfig {
 public:
  UntrustedPolkadotUIConfig();
  ~UntrustedPolkadotUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_UI_H_
