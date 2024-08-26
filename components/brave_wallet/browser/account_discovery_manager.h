// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ACCOUNT_DISCOVERY_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ACCOUNT_DISCOVERY_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

class BitcoinWalletService;
class KeyringService;
struct DiscoveredBitcoinAccount;

// Start account discovery process. Consecutively look for accounts with at
// least one transaction. Add such ones and all missing previous ones(so no
// gaps). Stop discovering when there are 20 consecutive accounts with no
// transactions.
class AccountDiscoveryManager {
 public:
  AccountDiscoveryManager(JsonRpcService* rpc_service,
                          KeyringService* keyring_service,
                          BitcoinWalletService* bitcoin_wallet_service);
  ~AccountDiscoveryManager();

  void StartDiscovery();

 private:
  struct DiscoveryContext {
    DiscoveryContext(const mojom::CoinType& coin_type,
                     const mojom::KeyringId& keyring_id,
                     const std::string& chain_id,
                     size_t discovery_account_index,
                     int attempts_left);
    ~DiscoveryContext();
    mojom::CoinType coin_type;
    mojom::KeyringId keyring_id;
    std::string chain_id;
    size_t discovery_account_index;
    int attempts_left;
  };

  std::map<mojom::KeyringId, uint32_t> GetDerivedAccountsCount();

  void AddDiscoveryAccount(std::unique_ptr<DiscoveryContext> context);

  void OnEthGetTransactionCount(std::unique_ptr<DiscoveryContext> context,
                                uint256_t result,
                                mojom::ProviderError error,
                                const std::string& error_message);
  void OnResolveAccountBalance(std::unique_ptr<DiscoveryContext> context,
                               const std::string& value,
                               mojom::ProviderError error,
                               const std::string& error_message);
  void OnResolveSolanaAccountBalance(std::unique_ptr<DiscoveryContext> context,
                                     uint64_t value,
                                     mojom::SolanaProviderError error,
                                     const std::string& error_message);
  void DiscoverBitcoinAccount(mojom::KeyringId keyring_id,
                              uint32_t account_index);
  void OnBitcoinDiscoverAccountsDone(
      mojom::KeyringId keyring_id,
      uint32_t account_index,
      base::expected<DiscoveredBitcoinAccount, std::string> discovered_account);

  void ProcessDiscoveryResult(std::unique_ptr<DiscoveryContext> context,
                              bool result);

  raw_ptr<brave_wallet::JsonRpcService> json_rpc_service_;
  raw_ptr<brave_wallet::KeyringService> keyring_service_;
  raw_ptr<brave_wallet::BitcoinWalletService> bitcoin_wallet_service_;

  base::WeakPtrFactory<AccountDiscoveryManager> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ACCOUNT_DISCOVERY_MANAGER_H_
