/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_block_tracker.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_state_manager.h"

namespace brave_wallet {

BitcoinTxManager::BitcoinTxManager(TxService* tx_service,
                                   JsonRpcService* json_rpc_service,
                                   BitcoinWalletService* bitcoin_wallet_service,
                                   KeyringService* keyring_service,
                                   PrefService* prefs)
    : TxManager(
          std::make_unique<BitcoinTxStateManager>(prefs, json_rpc_service),
          std::make_unique<BitcoinBlockTracker>(json_rpc_service,
                                                bitcoin_wallet_service),
          tx_service,
          json_rpc_service,
          keyring_service,
          prefs) {}

BitcoinTxManager::~BitcoinTxManager() = default;

void BitcoinTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& from,
    const absl::optional<url::Origin>& origin,
    const absl::optional<std::string>& group_id,
    AddUnapprovedTransactionCallback callback) {
  // TODO(apaymyshev): implement
}

void BitcoinTxManager::ApproveTransaction(const std::string& chain_id,
                                          const std::string& tx_meta_id,
                                          ApproveTransactionCallback callback) {
  // TODO(apaymyshev): implement
}

void BitcoinTxManager::GetAllTransactionInfo(
    const absl::optional<std::string>& chain_id,
    const absl::optional<std::string>& from,
    GetAllTransactionInfoCallback callback) {
  // TODO(apaymyshev): implement

  std::move(callback).Run({});
}

void BitcoinTxManager::SpeedupOrCancelTransaction(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void BitcoinTxManager::RetryTransaction(const std::string& chain_id,
                                        const std::string& tx_meta_id,
                                        RetryTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void BitcoinTxManager::GetTransactionMessageToSign(
    const std::string& chain_id,
    const std::string& tx_meta_id,
    GetTransactionMessageToSignCallback callback) {
  // TODO(apaymyshev): implement
}

void BitcoinTxManager::Reset() {
  TxManager::Reset();
}

mojom::CoinType BitcoinTxManager::GetCoinType() const {
  return mojom::CoinType::BTC;
}

void BitcoinTxManager::UpdatePendingTransactions(
    const absl::optional<std::string>& chain_id) {
  // TODO(apaymyshev): implement
}

}  // namespace brave_wallet
