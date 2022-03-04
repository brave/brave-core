/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_meta.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

FilTxMeta::FilTxMeta() : tx_(std::make_unique<FilTransaction>()) {}
FilTxMeta::FilTxMeta(std::unique_ptr<FilTransaction> tx) : tx_(std::move(tx)) {}

FilTxMeta::~FilTxMeta() = default;

bool FilTxMeta::operator==(const FilTxMeta& meta) const {
  return TxMeta::operator==(meta) && *tx_ == *meta.tx_;
}

base::Value FilTxMeta::ToValue() const {
  base::Value dict = TxMeta::ToValue();
  dict.SetKey("tx", tx_->ToValue());
  return dict;
}

mojom::TransactionInfoPtr FilTxMeta::ToTransactionInfo() const {
  return mojom::TransactionInfo::New(
      id_, from_, tx_hash_,
      mojom::TxDataUnion::NewFilTxData(mojom::FilTxData::New(
          tx_->nonce() ? base::NumberToString(*tx_->nonce()) : "",
          tx_->gas_premium(), tx_->gas_fee_cap(), tx_->max_fee(),
          base::NumberToString(tx_->gas_limit()), tx_->to().ToChecksumAddress(),
          tx_->value(), tx_->cid())),
      status_, mojom::TransactionType::Other,
      std::vector<std::string>() /* tx_params */,
      std::vector<std::string>() /* tx_args */,
      base::Milliseconds(created_time_.ToJavaTime()),
      base::Milliseconds(submitted_time_.ToJavaTime()),
      base::Milliseconds(confirmed_time_.ToJavaTime()));
}

}  // namespace brave_wallet
