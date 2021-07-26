// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

class WalletHandler : public brave_wallet::mojom::WalletHandler {
 public:
  WalletHandler(
      mojo::PendingReceiver<brave_wallet::mojom::WalletHandler> receiver,
      Profile* profile);

  WalletHandler(const WalletHandler&) = delete;
  WalletHandler& operator=(const WalletHandler&) = delete;
  ~WalletHandler() override;

  // brave_wallet::mojom::WalletHandler:
  void GetWalletInfo(GetWalletInfoCallback) override;
  void LockWallet() override;
  void UnlockWallet(const std::string& password, UnlockWalletCallback) override;
  void GetAssetPrice(const std::vector<std::string>& from_assets,
                     const std::vector<std::string>& to_assets,
                     GetAssetPriceCallback) override;
  void GetAssetPriceHistory(const std::string& asset,
                            brave_wallet::mojom::AssetPriceTimeframe timeframe,
                            GetAssetPriceHistoryCallback) override;

  void GetTokenByContract(const std::string& contract,
                          GetTokenByContractCallback) override;
  void GetTokenBySymbol(const std::string& symbol,
                        GetTokenBySymbolCallback) override;
  void GetAllTokens(GetAllTokensCallback) override;

  void GetPriceQuote(brave_wallet::mojom::SwapParamsPtr params,
                     GetPriceQuoteCallback) override;
  void GetTransactionPayload(brave_wallet::mojom::SwapParamsPtr params,
                             GetTransactionPayloadCallback) override;

  void AddFavoriteApp(brave_wallet::mojom::AppItemPtr app_item) override;
  void RemoveFavoriteApp(brave_wallet::mojom::AppItemPtr app_item) override;
  void NotifyWalletBackupComplete() override;
  void SetInitialAccountNames(
      const std::vector<std::string>& account_names) override;
  void AddNewAccountName(const std::string& account_name) override;

  void GetNetwork(GetNetworkCallback) override;
  void SetNetwork(brave_wallet::mojom::Network network) override;
  void GetChainId(GetChainIdCallback) override;
  void GetBlockTrackerUrl(GetBlockTrackerUrlCallback) override;
  void GetBalance(const std::string& address, GetBalanceCallback) override;
  void GetERC20TokenBalance(const std::string& contract,
                            const std::string& address,
                            GetERC20TokenBalanceCallback callback) override;

 private:
  void EnsureConnected();
  void OnConnectionError();

  void OnGetWalletInfo(GetWalletInfoCallback callback,
                       brave_wallet::mojom::KeyringInfoPtr keyring_info);

  mojo::Remote<brave_wallet::mojom::KeyringController> keyring_controller_;
  mojo::Remote<brave_wallet::mojom::AssetRatioController>
      asset_ratio_controller_;
  mojo::Remote<brave_wallet::mojom::SwapController> swap_controller_;
  mojo::Remote<brave_wallet::mojom::EthJsonRpcController> rpc_controller_;

  // TODO(bbondy): This needs to be persisted in prefs
  std::vector<brave_wallet::mojom::AppItemPtr> favorite_apps;
  mojo::Receiver<brave_wallet::mojom::WalletHandler> receiver_;

  Profile* profile_;  // NOT OWNED
  base::WeakPtrFactory<WalletHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_COMMON_HANDLER_WALLET_HANDLER_H_
