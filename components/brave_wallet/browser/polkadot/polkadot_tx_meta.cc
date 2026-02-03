/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

PolkadotTxMeta::PolkadotTxMeta() = default;
PolkadotTxMeta::~PolkadotTxMeta() = default;

base::DictValue PolkadotTxMeta::ToValue() const {
  auto dict = TxMeta::ToValue();
  dict.Set("tx", tx_->ToValue());
  return dict;
}

void PolkadotTxMeta::set_tx(PolkadotTransaction tx) {
  tx_.emplace(std::move(tx));
}

base::optional_ref<PolkadotTransaction> PolkadotTxMeta::tx() {
  return tx_;
}

mojom::TransactionInfoPtr PolkadotTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewPolkadotTxData(mojom::PolkadotTxdata::New(
          tx_->recipient().ToString().value_or(""),
          Uint128ToMojom(tx_->amount()), Uint128ToMojom(tx_->fee()), false)),
      status_, mojom::TransactionType::Other,
      std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_, "",
      false, swap_info_.Clone(), nullptr);
}

mojom::CoinType PolkadotTxMeta::GetCoinType() const {
  return mojom::CoinType::DOT;
}

}  // namespace brave_wallet
