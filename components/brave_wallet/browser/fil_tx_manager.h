/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;
class FilNonceTracker;
class FilTxStateManager;
class FilTransaction;

class FilTxManager : public TxManager, public FilBlockTracker::Observer {
 public:
  FilTxManager(TxService& tx_service,
               JsonRpcService* json_rpc_service,
               KeyringService& keyring_service,
               TxStorageDelegate& delegate,
               AccountResolverDelegate& account_resolver_delegate);
  ~FilTxManager() override;
  FilTxManager(const FilTxManager&) = delete;
  FilTxManager operator=(const FilTxManager&) = delete;

  using GetFilTransactionMessageToSignCallback =
      mojom::FilTxManagerProxy::GetFilTransactionMessageToSignCallback;
  using ProcessFilHardwareSignatureCallback =
      mojom::FilTxManagerProxy::ProcessFilHardwareSignatureCallback;

  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::TxDataUnionPtr tx_data_union,
                                const mojom::AccountIdPtr& from,
                                const std::optional<url::Origin>& origin,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void GetFilTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetFilTransactionMessageToSignCallback callback);

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void Reset() override;

  void ProcessFilHardwareSignature(
      const std::string& tx_meta_id,
      const mojom::FilecoinSignaturePtr& hw_signature,
      ProcessFilHardwareSignatureCallback callback);

  void GetEstimatedGas(const std::string& chain_id,
                       const mojom::AccountIdPtr& from_account_id,
                       const std::optional<url::Origin>& origin,
                       std::unique_ptr<FilTransaction> tx,
                       AddUnapprovedTransactionCallback callback);
  std::unique_ptr<FilTxMeta> GetTxForTesting(const std::string& tx_meta_id);

 private:
  friend class FilTxManagerUnitTest;

  mojom::CoinType GetCoinType() const override;

  void OnGetNextNonce(std::unique_ptr<FilTxMeta> meta,
                      ApproveTransactionCallback callback,
                      bool success,
                      uint256_t nonce);
  void OnGetNextNonceForHardware(
      std::unique_ptr<FilTxMeta> meta,
      GetFilTransactionMessageToSignCallback callback,
      bool success,
      uint256_t nonce);
  void OnSendFilecoinTransaction(const std::string& tx_meta_id,
                                 ApproveTransactionCallback callback,
                                 const std::string& tx_hash,
                                 mojom::FilecoinProviderError error,
                                 const std::string& error_message);
  void ContinueAddUnapprovedTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from_account_id,
      const std::optional<url::Origin>& origin,
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
  FilTxStateManager& GetFilTxStateManager();
  FilBlockTracker& GetFilBlockTracker();

  // FilBlockTracker::Observer
  void OnLatestHeightUpdated(const std::string& chain_id,
                             uint64_t latest_height) override;

  // TxManager
  void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) override;

  std::unique_ptr<FilNonceTracker> nonce_tracker_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  base::WeakPtrFactory<FilTxManager> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_MANAGER_H_
