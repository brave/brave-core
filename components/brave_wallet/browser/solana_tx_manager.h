/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this Solanae,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_

#include <string>

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"

class PrefService;

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;

class SolanaTxManager : public TxManager, public SolanaBlockTracker::Observer {
 public:
  SolanaTxManager(TxService* tx_service,
                  JsonRpcService* json_rpc_service,
                  KeyringService* keyring_service,
                  PrefService* prefs);
  ~SolanaTxManager() override;

  void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& from,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void RejectTransaction(const std::string& tx_meta_id,
                         RejectTransactionCallback) override;
  void GetAllTransactionInfo(const std::string& from,
                             GetAllTransactionInfoCallback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  void Reset() override;

 private:
  // TxManager
  void UpdatePendingTransactions() override;

  // SolanaBlockTracker::Observer
  void OnLatestBlockhashUpdated(const std::string& blockhash) override;

  SolanaBlockTracker* GetSolanaBlockTracker();
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_
