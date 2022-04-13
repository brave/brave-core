/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_meta.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

EthTxMeta::EthTxMeta() : tx_(std::make_unique<EthTransaction>()) {}
EthTxMeta::EthTxMeta(std::unique_ptr<EthTransaction> tx) : tx_(std::move(tx)) {}

EthTxMeta::~EthTxMeta() = default;

bool EthTxMeta::operator==(const EthTxMeta& meta) const {
  return TxMeta::operator==(meta) && tx_receipt_ == meta.tx_receipt_ &&
         *tx_ == *meta.tx_;
}

base::Value EthTxMeta::ToValue() const {
  base::Value dict = TxMeta::ToValue();
  dict.SetKey("tx_receipt", TransactionReceiptToValue(tx_receipt_));
  dict.SetKey("tx", tx_->ToValue());
  return dict;
}

mojom::TransactionInfoPtr EthTxMeta::ToTransactionInfo() const {
  std::string chain_id;
  std::string max_priority_fee_per_gas;
  std::string max_fee_per_gas;

  mojom::GasEstimation1559Ptr gas_estimation_1559_ptr = nullptr;
  if (tx_->type() == 1) {
    // When type is 1 it's always Eip2930Transaction
    auto* tx2930 = static_cast<Eip2930Transaction*>(tx_.get());
    chain_id = Uint256ValueToHex(tx2930->chain_id());
  } else if (tx_->type() == 2) {
    // When type is 2 it's always Eip1559Transaction
    auto* tx1559 = static_cast<Eip1559Transaction*>(tx_.get());
    chain_id = Uint256ValueToHex(tx1559->chain_id());
    max_priority_fee_per_gas =
        Uint256ValueToHex(tx1559->max_priority_fee_per_gas());
    max_fee_per_gas = Uint256ValueToHex(tx1559->max_fee_per_gas());
    gas_estimation_1559_ptr =
        Eip1559Transaction::GasEstimation::ToMojomGasEstimation1559(
            tx1559->gas_estimation());
  }

  mojom::TransactionType tx_type;
  std::vector<std::string> tx_params;
  std::vector<std::string> tx_args;
  std::string data = "0x0";
  if (tx_->data().size() > 0) {
    data = "0x" + base::HexEncode(tx_->data());
  }
  if (!GetTransactionInfoFromData(data, &tx_type, &tx_params, &tx_args)) {
    LOG(ERROR) << "Error parsing transaction data: " << data;
  }

  return mojom::TransactionInfo::New(
      id_, from_, tx_hash_,
      mojom::TxDataUnion::NewEthTxData1559(mojom::TxData1559::New(
          mojom::TxData::New(
              tx_->nonce() ? Uint256ValueToHex(tx_->nonce().value()) : "",
              Uint256ValueToHex(tx_->gas_price()),
              Uint256ValueToHex(tx_->gas_limit()),
              tx_->to().ToChecksumAddress(), Uint256ValueToHex(tx_->value()),
              tx_->data()),
          chain_id, max_priority_fee_per_gas, max_fee_per_gas,
          std::move(gas_estimation_1559_ptr))),
      status_, tx_type, tx_params, tx_args,
      base::Milliseconds(created_time_.ToJavaTime()),
      base::Milliseconds(submitted_time_.ToJavaTime()),
      base::Milliseconds(confirmed_time_.ToJavaTime()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr);
}

}  // namespace brave_wallet
