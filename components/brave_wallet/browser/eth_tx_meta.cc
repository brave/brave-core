/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_meta.h"

#include <optional>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eip2930_transaction.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

std::optional<std::string> GetFinalRecipient(
    const std::string& chain_id,
    const std::string& base_to,
    mojom::TransactionType tx_type,
    const std::vector<std::string>& tx_args) {
  if (tx_type == mojom::TransactionType::ETHFilForwarderTransfer) {
    if (tx_args.empty()) {
      return std::nullopt;
    }
    std::vector<uint8_t> bytes;
    if (!PrefixedHexStringToBytes(tx_args.at(0), &bytes)) {
      return std::nullopt;
    }
    std::string fil_chain_id =
        chain_id == mojom::kFilecoinEthereumMainnetChainId
            ? mojom::kFilecoinMainnet
            : mojom::kFilecoinTestnet;
    auto fil_address = FilAddress::FromBytes(fil_chain_id, bytes);
    if (fil_address.IsEmpty()) {
      return std::nullopt;
    }
    return fil_address.EncodeAsString();
  }

  if (tx_type == mojom::TransactionType::ERC20Transfer) {
    if (tx_args.empty()) {
      return std::nullopt;
    }
    return tx_args.at(0);
  }

  if (tx_type == mojom::TransactionType::ERC721TransferFrom ||
      tx_type == mojom::TransactionType::ERC721SafeTransferFrom) {
    if (tx_args.size() < 2) {
      return std::nullopt;
    }
    // (address owner, address to, uint256 tokenId)
    return tx_args.at(1);
  }

  return base_to;
}

}  // namespace

EthTxMeta::EthTxMeta() : tx_(std::make_unique<EthTransaction>()) {}
EthTxMeta::EthTxMeta(const mojom::AccountIdPtr& from,
                     std::unique_ptr<EthTransaction> tx)
    : tx_(std::move(tx)) {
  DCHECK_EQ(from->coin, mojom::CoinType::ETH);
  set_from(from.Clone());
}

EthTxMeta::~EthTxMeta() = default;

bool EthTxMeta::operator==(const EthTxMeta& meta) const {
  return TxMeta::operator==(meta) && tx_receipt_ == meta.tx_receipt_ &&
         *tx_ == *meta.tx_;
}

base::Value::Dict EthTxMeta::ToValue() const {
  base::Value::Dict dict = TxMeta::ToValue();
  dict.Set("tx_receipt", TransactionReceiptToValue(tx_receipt_));
  dict.Set("tx", tx_->ToValue());
  dict.Set("sign_only", sign_only_);
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
  std::vector<uint8_t> data{0x0};
  if (tx_->data().size() > 0) {
    data = tx_->data();
  }

  auto tx_info = GetTransactionInfoFromData(data);
  std::optional<std::string> final_recipient;
  if (!tx_info) {
    LOG(ERROR) << "Error parsing transaction data: " << ToHex(data);
  } else {
    std::tie(tx_type, tx_params, tx_args) = *tx_info;
    final_recipient = GetFinalRecipient(chain_id, tx_->to().ToChecksumAddress(),
                                        tx_type, tx_args);
  }
  std::optional<std::string> signed_transaction;
  if (tx_->IsSigned()) {
    signed_transaction = tx_->GetSignedTransaction();
  }

  return mojom::TransactionInfo::New(
      id_, from_->address, from_.Clone(), tx_hash_,
      mojom::TxDataUnion::NewEthTxData1559(mojom::TxData1559::New(
          mojom::TxData::New(
              tx_->nonce() ? Uint256ValueToHex(tx_->nonce().value()) : "",
              Uint256ValueToHex(tx_->gas_price()),
              Uint256ValueToHex(tx_->gas_limit()),
              tx_->to().ToChecksumAddress(), Uint256ValueToHex(tx_->value()),
              tx_->data(), sign_only_, signed_transaction),
          chain_id, max_priority_fee_per_gas, max_fee_per_gas,
          std::move(gas_estimation_1559_ptr))),
      status_, tx_type, tx_params, tx_args,
      base::Milliseconds(created_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(submitted_time_.InMillisecondsSinceUnixEpoch()),
      base::Milliseconds(confirmed_time_.InMillisecondsSinceUnixEpoch()),
      origin_.has_value() ? MakeOriginInfo(*origin_) : nullptr, chain_id_,
      final_recipient, IsRetriable());
}

mojom::CoinType EthTxMeta::GetCoinType() const {
  return mojom::CoinType::ETH;
}

bool EthTxMeta::IsRetriable() const {
  return IsRetriableStatus(status_);
}

}  // namespace brave_wallet
