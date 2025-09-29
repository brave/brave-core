/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_META_H_

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

// Polkadot transaction metadata class
class PolkadotTxMeta : public TxMeta {
 public:
  PolkadotTxMeta();
  ~PolkadotTxMeta() override;

  PolkadotTxMeta(const PolkadotTxMeta&) = delete;
  PolkadotTxMeta& operator=(const PolkadotTxMeta&) = delete;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;
  mojom::CoinType GetCoinType() const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_META_H_
