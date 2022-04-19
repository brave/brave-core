/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

class PrefService;

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;
class FilNonceTracker;
class FilTxStateManager;
class FilTransaction;

class FilTxManager : public TxManager, public FilBlockTracker::Observer {
 public:
  FilTxManager(TxService* tx_service,
               JsonRpcService* json_rpc_service,
               KeyringService* keyring_service,
               PrefService* prefs);
  ~FilTxManager() override;
  FilTxManager(const FilTxManager&) = delete;
  FilTxManager operator=(const FilTxManager&) = delete;

  void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& from,
                                const absl::optional<url::Origin>& origin,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void GetAllTransactionInfo(const std::string& from,
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

  void GetEstimatedGas(const std::string& from,
                       const absl::optional<url::Origin>& origin,
                       std::unique_ptr<FilTransaction> tx,
                       AddUnapprovedTransactionCallback callback);
  std::unique_ptr<FilTxMeta> GetTxForTesting(const std::string& tx_meta_id);

 private:
  friend class FilTxManagerUnitTest;

  void OnGetNextNonce(std::unique_ptr<FilTxMeta> meta,
                      ApproveTransactionCallback callback,
                      bool success,
                      uint256_t nonce);
  void OnSendFilecoinTransaction(const std::string& tx_meta_id,
                                 ApproveTransactionCallback callback,
                                 const std::string& tx_hash,
                                 mojom::FilecoinProviderError error,
                                 const std::string& error_message);
  void ContinueAddUnapprovedTransaction(
      const std::string& from,
      const absl::optional<url::Origin>& origin,
      std::unique_ptr<FilTransaction> tx,
      AddUnapprovedTransactionCallback callback,
      const std::string& gas_premium,
      const std::string& gas_fee_cap,
      int64_t gas_limit,
      mojom::FilecoinProviderError error,
      const std::string& error_message);
  void OnGetFilStateSearchMsgLimited(const std::string& tx_meta_id,
                                     int64_t exit_code,
                                     mojom::FilecoinProviderError error,
                                     const std::string& error_message);
  FilTxStateManager* GetFilTxStateManager();
  FilBlockTracker* GetFilBlockTracker();

  // FilBlockTracker::Observer
  void OnLatestHeightUpdated(uint64_t latest_height) override;

  // TxManager
  void UpdatePendingTransactions() override;

  std::unique_ptr<FilNonceTracker> nonce_tracker_;
  base::WeakPtrFactory<FilTxManager> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_MANAGER_H_
