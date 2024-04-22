/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {

SolanaTxMeta::SolanaTxMeta() = default;

SolanaTxMeta::SolanaTxMeta(const mojom::AccountIdPtr& from,
                           std::unique_ptr<SolanaTransaction> tx)
    : tx_(std::move(tx)) {
  DCHECK_EQ(from->coin, mojom::CoinType::SOL);
  set_from(std::move(from));
}

SolanaTxMeta::~SolanaTxMeta() = default;

bool SolanaTxMeta::operator==(const SolanaTxMeta& meta) const {
  return TxMeta::operator==(meta) &&
         signature_status_ == meta.signature_status_ && *tx_ == *meta.tx_;
}

base::Value::Dict SolanaTxMeta::ToValue() const {
  base::Value::Dict dict = TxMeta::ToValue();
  dict.Set("tx", tx_->ToValue());
  dict.Set("signature_status", signature_status_.ToValue());
  return dict;
}

mojom::TransactionInfoPtr SolanaTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, from_->address, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewSolanaTxData(tx_->ToSolanaTxData()), status_,
      tx_->tx_type(), std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      std::nullopt, IsRetriable());
}

mojom::CoinType SolanaTxMeta::GetCoinType() const {
  return mojom::CoinType::SOL;
}

bool SolanaTxMeta::IsRetriable() const {
  if (!IsRetriableStatus(status_)) {
    return false;
  }

  // For swap transactions, we need to get a fresh quote and start over from UI
  // rather than directly retrying the transaction.
  if (tx_->tx_type() == mojom::TransactionType::SolanaSwap) {
    return false;
  }

  // Do not retry for transactions that are partially signed because this can
  // only be retried from dApp side to re-sign the transaction.
  if (!tx_->message()->UsesDurableNonce() && tx_->IsPartialSigned()) {
    return false;
  }

  return true;
}

}  // namespace brave_wallet
