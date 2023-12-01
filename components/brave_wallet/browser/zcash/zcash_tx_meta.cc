/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"

namespace brave_wallet {
namespace {
mojom::ZecTxDataPtr ToZecTxData(ZCashTransaction& tx) {
  std::vector<mojom::ZecTxInputPtr> mojom_inputs;
  for (auto& input : tx.inputs()) {
    mojom_inputs.push_back(
        mojom::ZecTxInput::New(input.utxo_address, input.utxo_value));
  }
  std::vector<mojom::ZecTxOutputPtr> mojom_outputs;
  for (auto& output : tx.outputs()) {
    mojom_outputs.push_back(
        mojom::ZecTxOutput::New(output.address, output.amount));
  }
  return mojom::ZecTxData::New(tx.to(), tx.amount(), tx.fee(),
                               std::move(mojom_inputs),
                               std::move(mojom_outputs));
}
}  // namespace

ZCashTxMeta::ZCashTxMeta() : tx_(std::make_unique<ZCashTransaction>()) {}
ZCashTxMeta::ZCashTxMeta(const mojom::AccountIdPtr& from,
                         std::unique_ptr<ZCashTransaction> tx)
    : tx_(std::move(tx)) {
  DCHECK_EQ(from->coin, mojom::CoinType::ZEC);
  set_from(from.Clone());
}

bool ZCashTxMeta::operator==(const ZCashTxMeta& other) const {
  return TxMeta::operator==(other) && *tx_ == *other.tx_;
}

ZCashTxMeta::~ZCashTxMeta() = default;

base::Value::Dict ZCashTxMeta::ToValue() const {
  base::Value::Dict dict = TxMeta::ToValue();
  dict.Set("tx", tx_->ToValue());
  return dict;
}

mojom::TransactionInfoPtr ZCashTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, std::nullopt, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewZecTxData(ToZecTxData(*tx_)), status_,
      mojom::TransactionType::Other, std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      tx_->to());
}

}  // namespace brave_wallet
