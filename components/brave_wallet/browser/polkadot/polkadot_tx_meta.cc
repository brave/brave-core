/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"

#include "base/notimplemented.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

PolkadotTxMeta::PolkadotTxMeta() = default;

PolkadotTxMeta::~PolkadotTxMeta() = default;

base::Value::Dict PolkadotTxMeta::ToValue() const {
  NOTIMPLEMENTED_LOG_ONCE();

  base::Value::Dict dict;
  return dict;
}

mojom::TransactionInfoPtr PolkadotTxMeta::ToTransactionInfo() const {
  NOTIMPLEMENTED_LOG_ONCE();

  auto tx_info = mojom::TransactionInfo::New();
  return tx_info;
}

mojom::CoinType PolkadotTxMeta::GetCoinType() const {
  NOTIMPLEMENTED_LOG_ONCE();

  return mojom::CoinType::DOT;
}

}  // namespace brave_wallet
