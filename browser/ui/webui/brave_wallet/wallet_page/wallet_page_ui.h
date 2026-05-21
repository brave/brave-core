/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_WALLET_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_WALLET_PAGE_UI_H_

#include <memory>

#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/wallet_handler.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/mojom/rewards_page.mojom.h"
#endif

namespace brave_wallet {
class WalletPageHandler;

class WalletPageUI : public ui::MojoWebUIController,
                     public mojom::PageHandlerFactory {
 public:
  explicit WalletPageUI(content::WebUI* web_ui);
  WalletPageUI(const WalletPageUI&) = delete;
  WalletPageUI& operator=(const WalletPageUI&) = delete;
  ~WalletPageUI() override;

  // Instantiates the implementor of the mojom::PageHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(mojo::PendingReceiver<mojom::PageHandlerFactory> receiver);

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  void BindInterface(
      mojo::PendingReceiver<brave_rewards::mojom::RewardsPageHandler> receiver);
#endif

 private:
  // mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingReceiver<mojom::PageHandler> page_receiver,
      mojo::PendingReceiver<mojom::WalletHandler> wallet_receiver,
      mojo::PendingReceiver<mojom::JsonRpcService> json_rpc_service,
      mojo::PendingReceiver<mojom::BitcoinWalletService> bitcoin_rpc_service,
      mojo::PendingReceiver<mojom::PolkadotWalletService> polkadot_rpc_service,
      mojo::PendingReceiver<mojom::ZCashWalletService> zcash_service,
      mojo::PendingReceiver<mojom::CardanoWalletService>
          cardano_wallet_service_receiver,
      mojo::PendingReceiver<mojom::SwapService> swap_service,
      mojo::PendingReceiver<mojom::AssetRatioService> asset_ratio_service,
      mojo::PendingReceiver<mojom::KeyringService> keyring_service,
      mojo::PendingReceiver<mojom::BlockchainRegistry> blockchain_registry,
      mojo::PendingReceiver<mojom::TxService> tx_service,
      mojo::PendingReceiver<mojom::EthTxManagerProxy> eth_tx_manager_proxy,
      mojo::PendingReceiver<mojom::SolanaTxManagerProxy>
          solana_tx_manager_proxy,
      mojo::PendingReceiver<mojom::FilTxManagerProxy> filecoin_tx_manager_proxy,
      mojo::PendingReceiver<mojom::BtcTxManagerProxy>
          bitcoin_tx_manager_proxy_receiver,
      mojo::PendingReceiver<mojom::BraveWalletService> brave_wallet_service,
      mojo::PendingReceiver<mojom::BraveWalletP3A> brave_wallet_p3a,
      mojo::PendingReceiver<mojom::IpfsService>
          brave_wallet_ipfs_service_receiver,
      mojo::PendingReceiver<mojom::MeldIntegrationService>
          meld_integration_service) override;

  std::unique_ptr<WalletPageHandler> page_handler_;
  std::unique_ptr<WalletHandler> wallet_handler_;
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  std::unique_ptr<brave_rewards::mojom::RewardsPageHandler> rewards_handler_;
#endif

  mojo::Receiver<mojom::PageHandlerFactory> page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class WalletPageUIConfig : public content::DefaultWebUIConfig<WalletPageUI> {
 public:
  WalletPageUIConfig();

  // WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};
}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_WALLET_PAGE_UI_H_
