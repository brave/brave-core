/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

class KeyringService;
class TxService;
class TxStorageDelegate;
class AccountResolverDelegate;

// Polkadot transaction manager
class PolkadotTxManager : public TxManager,
                          public PolkadotBlockTracker::Observer {
 public:
  PolkadotTxManager(TxService& tx_service,
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
      AddUnapprovedTransactionCallback callback) override;

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

  // PolkadotBlockTracker::Observer
  void OnLatestBlock(const std::string& chain_id, uint64_t block_num) override;
  void OnNewBlock(const std::string& chain_id, uint64_t block_num) override;

  // Helper methods
  PolkadotBlockTracker& GetPolkadotBlockTracker();
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_MANAGER_H_
