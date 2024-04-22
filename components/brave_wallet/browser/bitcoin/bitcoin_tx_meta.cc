/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {
namespace {
mojom::BtcTxDataPtr ToBtcTxData(BitcoinTransaction& tx) {
  std::vector<mojom::BtcTxInputPtr> mojom_inputs;
  for (auto& input : tx.inputs()) {
    mojom_inputs.push_back(mojom::BtcTxInput::New(
        input.utxo_address, base::HexEncode(input.utxo_outpoint.txid),
        input.utxo_outpoint.index, input.utxo_value));
  }
  std::vector<mojom::BtcTxOutputPtr> mojom_outputs;
  for (auto& output : tx.outputs()) {
    mojom_outputs.push_back(
        mojom::BtcTxOutput::New(output.address, output.amount));
  }
  return mojom::BtcTxData::New(tx.to(), tx.amount(), tx.sending_max_amount(),
                               tx.EffectiveFeeAmount(), std::move(mojom_inputs),
                               std::move(mojom_outputs));
}
}  // namespace

BitcoinTxMeta::BitcoinTxMeta() : tx_(std::make_unique<BitcoinTransaction>()) {}
BitcoinTxMeta::BitcoinTxMeta(const mojom::AccountIdPtr& from,
                             std::unique_ptr<BitcoinTransaction> tx)
    : tx_(std::move(tx)) {
  DCHECK_EQ(from->coin, mojom::CoinType::BTC);
  set_from(from.Clone());
}

bool BitcoinTxMeta::operator==(const BitcoinTxMeta& other) const {
  return TxMeta::operator==(other) && *tx_ == *other.tx_;
}

BitcoinTxMeta::~BitcoinTxMeta() = default;

base::Value::Dict BitcoinTxMeta::ToValue() const {
  base::Value::Dict dict = TxMeta::ToValue();
  dict.Set("tx", tx_->ToValue());
  return dict;
}

mojom::TransactionInfoPtr BitcoinTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, std::nullopt, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewBtcTxData(ToBtcTxData(*tx_)), status_,
      mojom::TransactionType::Other, std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      tx_->to(), false);
}

mojom::CoinType BitcoinTxMeta::GetCoinType() const {
  return mojom::CoinType::BTC;
}

}  // namespace brave_wallet
