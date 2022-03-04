/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this Solanae,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace brave_wallet {

class TxService;
class JsonRpcService;
class KeyringService;
class SolanaTxMeta;
class SolanaTxStateManager;
struct SolanaSignatureStatus;

class SolanaTxManager : public TxManager, public SolanaBlockTracker::Observer {
 public:
  SolanaTxManager(TxService* tx_service,
                  JsonRpcService* json_rpc_service,
                  KeyringService* keyring_service,
                  PrefService* prefs);
  ~SolanaTxManager() override;

  // TxManager
  void AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                const std::string& from,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) override;

  std::unique_ptr<SolanaTxMeta> GetTxForTesting(const std::string& tx_meta_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaTxManagerUnitTest, AddAndApproveTransaction);

  // TxManager
  void UpdatePendingTransactions() override;

  void OnGetLatestBlockhash(std::unique_ptr<SolanaTxMeta> meta,
                            ApproveTransactionCallback callback,
                            const std::string& latest_blockhash,
                            mojom::SolanaProviderError error,
                            const std::string& error_message);
  void OnSendSolanaTransaction(const std::string& tx_meta_id,
                               ApproveTransactionCallback callback,
                               const std::string& tx_hash,
                               mojom::SolanaProviderError error,
                               const std::string& error_message);
  void OnGetSignatureStatuses(
      const std::vector<std::string>& tx_meta_ids,
      const std::vector<absl::optional<SolanaSignatureStatus>>&
          signature_statuses,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  // SolanaBlockTracker::Observer
  void OnLatestBlockhashUpdated(const std::string& blockhash) override;

  SolanaTxStateManager* GetSolanaTxStateManager();
  SolanaBlockTracker* GetSolanaBlockTracker();

  base::WeakPtrFactory<SolanaTxManager> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_MANAGER_H_
