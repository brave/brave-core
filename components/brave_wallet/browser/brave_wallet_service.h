/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;
class PrefRegistrySimple;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_wallet {

class BraveWalletServiceObserver;
class EthJsonRpcController;
class EthTxController;
class KeyringController;
class AssetRatioController;
class SwapController;

class BraveWalletService : public KeyedService,
                           public base::SupportsWeakPtr<BraveWalletService> {
 public:
  explicit BraveWalletService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveWalletService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  brave_wallet::EthJsonRpcController* rpc_controller() const;
  brave_wallet::KeyringController* keyring_controller() const;
  brave_wallet::EthTxController* tx_controller() const;
  brave_wallet::AssetRatioController* asset_ratio_controller() const;
  brave_wallet::SwapController* swap_controller() const;

  std::vector<std::string> WalletAccountNames() const;
  void SetInitialAccountNames(const std::vector<std::string>& account_names);
  void AddNewAccountName(const std::string& account_name);
  bool IsWalletBackedUp() const;
  void NotifyWalletBackupComplete();
  void NotifyShowEthereumPermissionPrompt(
      int32_t tab_id,
      const std::vector<std::string>& accounts,
      const std::string& origin);

  void AddObserver(BraveWalletServiceObserver* observer);
  void RemoveObserver(BraveWalletServiceObserver* observer);

  struct PermissionRequest {
    explicit PermissionRequest(int32_t tab_id,
                               const std::vector<std::string>& accounts,
                               const std::string& origin);
    ~PermissionRequest();

    int32_t tab_id;
    std::vector<std::string> accounts;
    std::string origin;
  };

  void SetPanelHandlerReady(bool ready);

 private:
  PrefService* prefs_;
  std::unique_ptr<brave_wallet::EthJsonRpcController> rpc_controller_;
  std::unique_ptr<brave_wallet::KeyringController> keyring_controller_;
  std::unique_ptr<brave_wallet::EthTxController> tx_controller_;
  std::unique_ptr<brave_wallet::AssetRatioController> asset_ratio_controller_;
  std::unique_ptr<brave_wallet::SwapController> swap_controller_;
  std::unique_ptr<PermissionRequest> pending_permission_request_;

  bool panel_handler_ready_ = false;
  base::ObserverList<BraveWalletServiceObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(BraveWalletService);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
