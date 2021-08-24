/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <utility>

#include "base/base64.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"

namespace brave_wallet {

namespace {
constexpr uint256_t kContractCreationCost = 32000;
constexpr uint256_t kTransactionCost = 21000;
constexpr uint256_t kTxDataZeroCostPerByte = 4;
constexpr uint256_t kTxDataCostPerByte = 16;
}  // namespace

EthTransaction::EthTransaction() = default;
EthTransaction::EthTransaction(const EthTransaction&) = default;
EthTransaction::EthTransaction(uint256_t nonce,
                               uint256_t gas_price,
                               uint256_t gas_limit,
                               const EthAddress& to,
                               uint256_t value,
                               const std::vector<uint8_t>& data)
    : nonce_(nonce),
      gas_price_(gas_price),
      gas_limit_(gas_limit),
      to_(to),
      value_(value),
      data_(data) {}
EthTransaction::~EthTransaction() = default;

bool EthTransaction::operator==(const EthTransaction& tx) const {
  return nonce_ == tx.nonce_ && gas_price_ == tx.gas_price_ &&
         gas_limit_ == tx.gas_limit_ && to_ == tx.to_ && value_ == tx.value_ &&
         std::equal(data_.begin(), data_.end(), tx.data_.begin()) &&
         v_ == tx.v_ && std::equal(r_.begin(), r_.end(), tx.r_.begin()) &&
         std::equal(s_.begin(), s_.end(), tx.s_.begin()) && type_ == tx.type_;
}

// static
absl::optional<EthTransaction> EthTransaction::FromTxData(
    const mojom::TxDataPtr& tx_data) {
  EthTransaction tx;
  if (!HexValueToUint256(tx_data->nonce, &tx.nonce_))
    return absl::nullopt;
  if (!HexValueToUint256(tx_data->gas_price, &tx.gas_price_))
    return absl::nullopt;
  if (!HexValueToUint256(tx_data->gas_limit, &tx.gas_limit_))
    return absl::nullopt;
  tx.to_ = EthAddress::FromHex(tx_data->to);
  if (!HexValueToUint256(tx_data->value, &tx.value_))
    return absl::nullopt;
  tx.data_ = tx_data->data;
  return tx;
}

// static
absl::optional<EthTransaction> EthTransaction::FromValue(
    const base::Value& value) {
  EthTransaction tx;
  const std::string* nonce = value.FindStringKey("nonce");
  if (!nonce)
    return absl::nullopt;
  if (!HexValueToUint256(*nonce, &tx.nonce_))
    return absl::nullopt;

  const std::string* gas_price = value.FindStringKey("gas_price");
  if (!gas_price)
    return absl::nullopt;
  if (!HexValueToUint256(*gas_price, &tx.gas_price_))
    return absl::nullopt;

  const std::string* gas_limit = value.FindStringKey("gas_limit");
  if (!gas_limit)
    return absl::nullopt;
  if (!HexValueToUint256(*gas_limit, &tx.gas_limit_))
    return absl::nullopt;

  const std::string* to = value.FindStringKey("to");
  if (!to)
    return absl::nullopt;
  tx.to_ = EthAddress::FromHex(*to);

  const std::string* tx_value = value.FindStringKey("value");
  if (!tx_value)
    return absl::nullopt;
  if (!HexValueToUint256(*tx_value, &tx.value_))
    return absl::nullopt;

  const std::string* data = value.FindStringKey("data");
  if (!data)
    return absl::nullopt;
  std::string data_decoded;
  if (!base::Base64Decode(*data, &data_decoded))
    return absl::nullopt;
  tx.data_ = std::vector<uint8_t>(data_decoded.begin(), data_decoded.end());

  absl::optional<int> v = value.FindIntKey("v");
  if (!v)
    return absl::nullopt;
  tx.v_ = (uint8_t)*v;

  const std::string* r = value.FindStringKey("r");
  if (!r)
    return absl::nullopt;
  std::string r_decoded;
  if (!base::Base64Decode(*r, &r_decoded))
    return absl::nullopt;
  tx.r_ = std::vector<uint8_t>(r_decoded.begin(), r_decoded.end());

  const std::string* s = value.FindStringKey("s");
  if (!s)
    return absl::nullopt;
  std::string s_decoded;
  if (!base::Base64Decode(*s, &s_decoded))
    return absl::nullopt;
  tx.s_ = std::vector<uint8_t>(s_decoded.begin(), s_decoded.end());

  absl::optional<int> type = value.FindIntKey("type");
  if (!type)
    return absl::nullopt;
  tx.type_ = (uint8_t)*type;

  return tx;
}

std::vector<uint8_t> EthTransaction::GetMessageToSign(
    uint256_t chain_id) const {
  base::ListValue list;
  list.Append(RLPUint256ToBlobValue(nonce_));
  list.Append(RLPUint256ToBlobValue(gas_price_));
  list.Append(RLPUint256ToBlobValue(gas_limit_));
  list.Append(base::Value(to_.bytes()));
  list.Append(RLPUint256ToBlobValue(value_));
  list.Append(base::Value(data_));
  if (chain_id) {
    list.Append(RLPUint256ToBlobValue(chain_id));
    list.Append(RLPUint256ToBlobValue(0));
    list.Append(RLPUint256ToBlobValue(0));
  }

  const std::string message = RLPEncode(std::move(list));
  return KeccakHash(std::vector<uint8_t>(message.begin(), message.end()));
}

std::string EthTransaction::GetSignedTransaction() const {
  base::ListValue list;
  list.Append(RLPUint256ToBlobValue(nonce_));
  list.Append(RLPUint256ToBlobValue(gas_price_));
  list.Append(RLPUint256ToBlobValue(gas_limit_));
  list.Append(base::Value(to_.bytes()));
  list.Append(RLPUint256ToBlobValue(value_));
  list.Append(base::Value(data_));
  list.Append(RLPUint256ToBlobValue(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  return ToHex(RLPEncode(std::move(list)));
}

// signature and recid will be used to produce v, r, s
void EthTransaction::ProcessSignature(const std::vector<uint8_t> signature,
                                      int recid,
                                      uint256_t chain_id) {
  if (signature.size() != 64) {
    LOG(ERROR) << __func__ << ": signature length should be 64 bytes";
    return;
  }
  if (recid < 0 || recid > 3) {
    LOG(ERROR) << __func__ << ": recovery id must be 0, 1, 2 or 3";
    return;
  }
  r_ = std::vector<uint8_t>(signature.begin(),
                            signature.begin() + signature.size() / 2);
  s_ = std::vector<uint8_t>(signature.begin() + signature.size() / 2,
                            signature.end());
  v_ = chain_id ? static_cast<uint256_t>(recid) +
                      (chain_id * static_cast<uint256_t>(2) +
                       static_cast<uint256_t>(35))
                : static_cast<uint256_t>(recid) + static_cast<uint256_t>(27);
}

bool EthTransaction::IsSigned() const {
  return v_ != (uint256_t)0 && r_.size() != 0 && s_.size() != 0;
}

base::Value EthTransaction::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("nonce", Uint256ValueToHex(nonce_));
  dict.SetStringKey("gas_price", Uint256ValueToHex(gas_price_));
  dict.SetStringKey("gas_limit", Uint256ValueToHex(gas_limit_));
  dict.SetStringKey("to", to_.ToHex());
  dict.SetStringKey("value", Uint256ValueToHex(value_));
  dict.SetStringKey("data", base::Base64Encode(data_));
  dict.SetIntKey("v", static_cast<int>(v_));
  dict.SetStringKey("r", base::Base64Encode(r_));
  dict.SetStringKey("s", base::Base64Encode(s_));
  dict.SetIntKey("type", static_cast<int>(type_));

  return dict;
}

uint256_t EthTransaction::GetBaseFee() const {
  uint256_t fee = GetDataFee() + kTransactionCost;
  if (IsToCreationAddress())
    fee += kContractCreationCost;

  return fee;
}

uint256_t EthTransaction::GetDataFee() const {
  uint256_t cost = 0;
  for (uint8_t byte : data_) {
    cost += byte == 0 ? kTxDataZeroCostPerByte : kTxDataCostPerByte;
  }
  return cost;
}

uint256_t EthTransaction::GetUpfrontCost(uint256_t block_base_fee) const {
  return gas_limit_ * gas_price_ + value_;
}

}  // namespace brave_wallet
