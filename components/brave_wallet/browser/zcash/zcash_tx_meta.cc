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
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {
namespace {
mojom::ZecTxDataPtr ToZecTxData(const std::string& chain_id,
                                ZCashTransaction& tx) {
  std::vector<mojom::ZecTxInputPtr> mojom_inputs;
  for (auto& input : tx.transparent_part().inputs) {
    mojom_inputs.push_back(
        mojom::ZecTxInput::New(input.utxo_address, input.utxo_value));
  }
  std::vector<mojom::ZecTxOutputPtr> mojom_outputs;
  for (auto& output : tx.transparent_part().outputs) {
    mojom_outputs.push_back(
        mojom::ZecTxOutput::New(output.address, output.amount));
  }
  for (auto& output : tx.orchard_part().outputs) {
    auto orchard_unified_addr =
        GetOrchardUnifiedAddress(output.addr, chain_id == mojom::kZCashTestnet);
    if (orchard_unified_addr) {
      mojom_outputs.push_back(
          mojom::ZecTxOutput::New(*orchard_unified_addr, output.value));
    }
  }
  // TODO(cypt4): Add proper flag here
  // https://github.com/brave/brave-browser/issues/39314
  return mojom::ZecTxData::New(false, tx.to(), OrchardMemoToVec(tx.memo()),
                               tx.amount(), tx.fee(), std::move(mojom_inputs),
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
      mojom::TxDataUnion::NewZecTxData(ToZecTxData(chain_id_, *tx_)), status_,
      mojom::TransactionType::Other, std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      tx_->to(), false, nullptr);
}

mojom::CoinType ZCashTxMeta::GetCoinType() const {
  return mojom::CoinType::ZEC;
}

}  // namespace brave_wallet
