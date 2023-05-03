/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_state_manager.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"

namespace brave_wallet {

BitcoinTxStateManager::BitcoinTxStateManager(PrefService* prefs,
                                             JsonRpcService* json_rpc_service)
    : TxStateManager(prefs) {}

BitcoinTxStateManager::~BitcoinTxStateManager() = default;

std::string BitcoinTxStateManager::GetTxPrefPathPrefix(
    const absl::optional<std::string>& chain_id) {
  // TODO(apaymyshev): implement

  return "";
}

mojom::CoinType BitcoinTxStateManager::GetCoinType() const {
  return mojom::CoinType::BTC;
}

std::unique_ptr<TxMeta> BitcoinTxStateManager::ValueToTxMeta(
    const base::Value::Dict& value) {
  // TODO(apaymyshev): implement

  return nullptr;
}

}  // namespace brave_wallet
