/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_MANAGER_H_

#include <optional>
#include <string>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

class KeyringService;
class TxService;
class TxStorageDelegate;
class AccountResolverDelegate;
class PolkadotWalletService;

// Polkadot transaction manager
class PolkadotTxManager : public TxManager,
                          public PolkadotBlockTracker::Observer {
 public:
  using AddUnapprovedPolkadotTransactionCallback =
      mojom::TxService::AddUnapprovedPolkadotTransactionCallback;

  PolkadotTxManager(TxService& tx_service,
                    PolkadotWalletService& polkadot_wallet_service,
                    KeyringService& keyring_service,
                    TxStorageDelegate& delegate,
                    AccountResolverDelegate& account_resolver_delegate);
  ~PolkadotTxManager() override;
  PolkadotTxManager(const PolkadotTxManager&) = delete;
  PolkadotTxManager& operator=(const PolkadotTxManager&) = delete;

  // TxManager
  void AddUnapprovedTransaction(
      const std::string& chain_id,
      mojom::TxDataUnionPtr tx_data_union,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      mojom::SwapInfoPtr swap_info,
      AddUnapprovedTransactionCallback callback) override;

  void AddUnapprovedPolkadotTransaction(
      mojom::NewPolkadotTransactionParamsPtr params,
      AddUnapprovedPolkadotTransactionCallback callback);

  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback callback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;

  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;

  void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) override;

  void Reset() override;

  mojom::CoinType GetCoinType() const override;

 private:
  friend class PolkadotTxManagerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(PolkadotTxManagerUnitTest, OnLatestBlock);
  FRIEND_TEST_ALL_PREFIXES(PolkadotTxManagerUnitTest, OnNewBlock);

  void OnGetChainMetadataForUnapproved(
      mojom::NewPolkadotTransactionParamsPtr params,
      AddUnapprovedPolkadotTransactionCallback callback,
      const base::expected<PolkadotChainMetadata, std::string>& chain_metadata);

  // PolkadotBlockTracker::Observer
  void OnLatestBlock(const std::string& chain_id, uint64_t block_num) override;
  void OnNewBlock(const std::string& chain_id, uint64_t block_num) override;

  // Helper methods
  PolkadotBlockTracker& GetPolkadotBlockTracker();

  raw_ref<PolkadotWalletService> polkadot_wallet_service_;

  base::WeakPtrFactory<PolkadotTxManager> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_MANAGER_H_
