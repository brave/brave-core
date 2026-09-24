/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {
namespace {

void AppendShieldedPool(const std::string& chain_id,
                        const ZCashTransaction::ShieldedPool& pool_data,
                        mojom::ZCashTokenType pool_tag,
                        std::vector<mojom::ZecTxInputPtr>& inputs,
                        std::vector<mojom::ZecTxOutputPtr>& outputs) {
  const bool testnet = chain_id == mojom::kZCashTestnet;
  for (const auto& in : pool_data.inputs) {
    auto ua = GetOrchardUnifiedAddress(in.note.addr, testnet);
    if (ua) {
      inputs.push_back(mojom::ZecTxInput::New(*ua, in.note.amount, pool_tag));
    }
  }
  for (const auto& out : pool_data.outputs) {
    auto ua = GetOrchardUnifiedAddress(out.addr, testnet);
    if (ua) {
      outputs.push_back(mojom::ZecTxOutput::New(*ua, out.value, pool_tag));
    }
  }
}

mojom::ZecTxDataPtr ToZecTxData(const std::string& chain_id,
                                ZCashTransaction& tx) {
  std::vector<mojom::ZecTxInputPtr> mojom_inputs;
  std::vector<mojom::ZecTxOutputPtr> mojom_outputs;

  // Transparent part is version-agnostic.
  for (auto& input : tx.transparent_part().inputs) {
    mojom_inputs.push_back(
        mojom::ZecTxInput::New(input.utxo_address, input.utxo_value,
                               mojom::ZCashTokenType::kTransparent));
  }
  for (auto& output : tx.transparent_part().outputs) {
    mojom_outputs.push_back(mojom::ZecTxOutput::New(
        output.address, output.amount, mojom::ZCashTokenType::kTransparent));
  }

  mojom::ZCashTokenType zcash_token_type = mojom::ZCashTokenType::kTransparent;
  if (tx.is_v6()) {
    AppendShieldedPool(chain_id, tx.v6_part().legacy_orchard,
                       mojom::ZCashTokenType::kOrchard, mojom_inputs,
                       mojom_outputs);
    AppendShieldedPool(chain_id, tx.v6_part().ironwood,
                       mojom::ZCashTokenType::kIronwood, mojom_inputs,
                       mojom_outputs);
    CHECK(tx.v6_part().legacy_orchard.inputs.empty() ||
          tx.v6_part().ironwood.inputs.empty())
        << "ZecTxData can represent only one source shielded pool";
    if (!tx.v6_part().ironwood.inputs.empty()) {
      zcash_token_type = mojom::ZCashTokenType::kIronwood;
    } else if (!tx.v6_part().legacy_orchard.inputs.empty()) {
      zcash_token_type = mojom::ZCashTokenType::kOrchard;
    }
  } else {
    AppendShieldedPool(chain_id, tx.v5_part().orchard,
                       mojom::ZCashTokenType::kOrchard, mojom_inputs,
                       mojom_outputs);
    if (!tx.v5_part().orchard.inputs.empty()) {
      zcash_token_type = mojom::ZCashTokenType::kOrchard;
    }
  }

  return mojom::ZecTxData::New(
      zcash_token_type, tx.to(), false, OrchardMemoToVec(tx.memo()),
      tx.amount(), tx.fee(), std::move(mojom_inputs), std::move(mojom_outputs));
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

base::DictValue ZCashTxMeta::ToValue() const {
  base::DictValue dict = TxMeta::ToValue();
  if (tx_) {
    dict.Set("tx", tx_->ToValue());
  }
  return dict;
}

mojom::TransactionInfoPtr ZCashTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewZecTxData(ToZecTxData(chain_id_, *tx_)), status_,
      mojom::TransactionType::Other, std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      tx_->to(), false, swap_info_.Clone(), nullptr);
}

mojom::CoinType ZCashTxMeta::GetCoinType() const {
  return mojom::CoinType::ZEC;
}

}  // namespace brave_wallet
