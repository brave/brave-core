/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TX_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"

class PrefService;

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;
class BitcoinWalletService;

class BitcoinTxManager : public TxManager {
 public:
  BitcoinTxManager(TxService* tx_service,
                   JsonRpcService* json_rpc_service,
                   BitcoinWalletService* bitcoin_wallet_service,
                   KeyringService* keyring_service,
                   PrefService* prefs);
  ~BitcoinTxManager() override;
  BitcoinTxManager(const BitcoinTxManager&) = delete;
  BitcoinTxManager& operator=(const BitcoinTxManager&) = delete;

  void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& from,
                                const absl::optional<url::Origin>& origin,
                                const absl::optional<std::string>& group_id,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void GetAllTransactionInfo(const absl::optional<std::string>& from,
                             GetAllTransactionInfoCallback) override;
  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void Reset() override;

 private:
  // TxManager
  void UpdatePendingTransactions() override;

  base::WeakPtrFactory<BitcoinTxManager> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_TX_MANAGER_H_
