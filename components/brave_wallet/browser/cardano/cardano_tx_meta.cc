/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace {

mojom::CardanoTxDataPtr ToCardanoTxData(const CardanoTransaction& tx) {
  std::vector<mojom::CardanoTxInputPtr> mojom_inputs;
  for (auto& input : tx.inputs()) {
    mojom_inputs.push_back(mojom::CardanoTxInput::New(
        input.utxo_address.ToString(),
        base::HexEncode(input.utxo_outpoint.txid), input.utxo_outpoint.index,
        input.utxo_value, std::vector<mojom::CardanoTxTokenValuePtr>()));
    for (auto& token : input.utxo_tokens) {
      mojom_inputs.back()->tokens.push_back(mojom::CardanoTxTokenValue::New(
          base::HexEncodeLower(token.first), token.second));
    }
  }

  std::vector<mojom::CardanoTxOutputPtr> mojom_outputs;
  for (auto& output : tx.outputs()) {
    mojom_outputs.push_back(mojom::CardanoTxOutput::New(
        output.address.ToString(), output.amount,
        std::vector<mojom::CardanoTxTokenValuePtr>()));
    for (auto& token : output.tokens) {
      mojom_outputs.back()->tokens.push_back(mojom::CardanoTxTokenValue::New(
          base::HexEncodeLower(token.first), token.second));
    }
  }

  mojom::CardanoTxTokenValuePtr sending_token;
  CHECK(tx.TargetOutput());
  uint64_t sending_lovelace = tx.TargetOutput()->amount;
  if (!tx.TargetOutput()->tokens.empty()) {
    CHECK_EQ(tx.TargetOutput()->tokens.size(), 1u);
    const auto& token = tx.TargetOutput()->tokens.begin();
    sending_token = mojom::CardanoTxTokenValue::New(
        base::HexEncodeLower(token->first), token->second);
  }

  std::string send_to_address_str;
  if (auto send_to_address = tx.GetToAddress()) {
    send_to_address_str = send_to_address->ToString();
  }

  return mojom::CardanoTxData::New(
      send_to_address_str, sending_lovelace, std::move(sending_token), tx.fee(),
      std::move(mojom_inputs), std::move(mojom_outputs));
}

}  // namespace

CardanoTxMeta::CardanoTxMeta() : tx_(std::make_unique<CardanoTransaction>()) {}
CardanoTxMeta::CardanoTxMeta(const mojom::AccountIdPtr& from,
                             std::unique_ptr<CardanoTransaction> tx)
    : tx_(std::move(tx)) {
  DCHECK_EQ(from->coin, mojom::CoinType::ADA);
  set_from(from.Clone());
}

bool CardanoTxMeta::operator==(const CardanoTxMeta& other) const {
  return TxMeta::operator==(other) && *tx_ == *other.tx_;
}

CardanoTxMeta::~CardanoTxMeta() = default;

base::DictValue CardanoTxMeta::ToValue() const {
  base::DictValue dict = TxMeta::ToValue();
  dict.Set("tx", tx_->ToValue());
  return dict;
}

mojom::TransactionInfoPtr CardanoTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewCardanoTxData(ToCardanoTxData(*tx_)), status_,
      tx_->IsSendTokenTransaction()
          ? mojom::TransactionType::CardanoSendToken
          : mojom::TransactionType::CardanoSendLovelace,
      std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      std::nullopt /* effective_recipient */, false /* is_retriable */,
      swap_info_.Clone(), nullptr);
}

mojom::CoinType CardanoTxMeta::GetCoinType() const {
  return mojom::CoinType::ADA;
}

}  // namespace brave_wallet
