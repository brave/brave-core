/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/containers/to_vector.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {
constexpr uint256_t kContractCreationCost = 32000;
constexpr uint256_t kTransactionCost = 21000;
constexpr uint256_t kTxDataZeroCostPerByte = 4;
constexpr uint256_t kTxDataCostPerByte = 16;
}  // namespace

EthTransaction::EthTransaction() : gas_price_(0), gas_limit_(0), value_(0) {}

EthTransaction::EthTransaction(const EthTransaction&) = default;
EthTransaction::EthTransaction(std::optional<uint256_t> nonce,
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
std::optional<EthTransaction> EthTransaction::FromTxData(
    const mojom::TxDataPtr& tx_data,
    bool strict) {
  EthTransaction tx;
  if (!tx_data->nonce.empty()) {
    uint256_t nonce_uint;
    if (HexValueToUint256(tx_data->nonce, &nonce_uint)) {
      tx.nonce_ = nonce_uint;
    } else if (strict) {
      return std::nullopt;
    }
  }

  if (!HexValueToUint256(tx_data->gas_price, &tx.gas_price_) && strict) {
    return std::nullopt;
  }
  if (!HexValueToUint256(tx_data->gas_limit, &tx.gas_limit_) && strict) {
    return std::nullopt;
  }
  tx.to_ = EthAddress::FromHex(tx_data->to);
  if (!HexValueToUint256(tx_data->value, &tx.value_) && strict) {
    return std::nullopt;
  }
  tx.data_ = tx_data->data;
  return tx;
}

// static
std::optional<EthTransaction> EthTransaction::FromValue(
    const base::Value::Dict& value) {
  EthTransaction tx;
  const std::string* nonce = value.FindString("nonce");
  if (!nonce) {
    return std::nullopt;
  }

  if (!nonce->empty()) {
    uint256_t nonce_uint;
    if (!HexValueToUint256(*nonce, &nonce_uint)) {
      return std::nullopt;
    }
    tx.nonce_ = nonce_uint;
  }

  const std::string* gas_price = value.FindString("gas_price");
  if (!gas_price) {
    return std::nullopt;
  }
  if (!HexValueToUint256(*gas_price, &tx.gas_price_)) {
    return std::nullopt;
  }

  const std::string* gas_limit = value.FindString("gas_limit");
  if (!gas_limit) {
    return std::nullopt;
  }
  if (!HexValueToUint256(*gas_limit, &tx.gas_limit_)) {
    return std::nullopt;
  }

  const std::string* to = value.FindString("to");
  if (!to) {
    return std::nullopt;
  }
  tx.to_ = EthAddress::FromHex(*to);

  const std::string* tx_value = value.FindString("value");
  if (!tx_value) {
    return std::nullopt;
  }
  if (!HexValueToUint256(*tx_value, &tx.value_)) {
    return std::nullopt;
  }

  const std::string* data = value.FindString("data");
  if (!data) {
    return std::nullopt;
  }
  std::string data_decoded;
  if (!base::Base64Decode(*data, &data_decoded)) {
    return std::nullopt;
  }
  tx.data_ = std::vector<uint8_t>(data_decoded.begin(), data_decoded.end());

  std::optional<int> v = value.FindInt("v");
  if (!v) {
    return std::nullopt;
  }
  tx.v_ = (uint8_t)*v;

  const std::string* r = value.FindString("r");
  if (!r) {
    return std::nullopt;
  }
  std::string r_decoded;
  if (!base::Base64Decode(*r, &r_decoded)) {
    return std::nullopt;
  }
  tx.r_ = std::vector<uint8_t>(r_decoded.begin(), r_decoded.end());

  const std::string* s = value.FindString("s");
  if (!s) {
    return std::nullopt;
  }
  std::string s_decoded;
  if (!base::Base64Decode(*s, &s_decoded)) {
    return std::nullopt;
  }
  tx.s_ = std::vector<uint8_t>(s_decoded.begin(), s_decoded.end());

  std::optional<int> type = value.FindInt("type");
  if (!type) {
    return std::nullopt;
  }
  tx.type_ = (uint8_t)*type;

  return tx;
}

std::vector<uint8_t> EthTransaction::GetMessageToSign(uint256_t chain_id,
                                                      bool hash) const {
  DCHECK(nonce_);
  base::Value::List list;
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(gas_price_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(to_.bytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  if (chain_id) {
    list.Append(RLPUint256ToBlob(chain_id));
    list.Append(RLPUint256ToBlob(0));
    list.Append(RLPUint256ToBlob(0));
  }

  auto result = RLPEncode(list);
  return hash ? base::ToVector(KeccakHash(result)) : result;
}

std::string EthTransaction::GetSignedTransaction() const {
  DCHECK(nonce_);

  return ToHex(RLPEncode(Serialize()));
}

std::string EthTransaction::GetTransactionHash() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(KeccakHash(base::as_byte_span(RLPEncode(Serialize()))));
}

bool EthTransaction::ProcessVRS(const std::vector<uint8_t>& v,
                                const std::vector<uint8_t>& r,
                                const std::vector<uint8_t>& s) {
  if (r.empty() || s.empty() || v.empty()) {
    return false;
  }

  if (!HexValueToUint256(ToHex(v), &v_)) {
    LOG(ERROR) << "Unable to decode v param";
    return false;
  }

  r_ = r;
  s_ = s;
  return true;
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

base::Value::Dict EthTransaction::ToValue() const {
  base::Value::Dict dict;
  dict.Set("nonce", nonce_ ? Uint256ValueToHex(nonce_.value()) : "");
  dict.Set("gas_price", Uint256ValueToHex(gas_price_));
  dict.Set("gas_limit", Uint256ValueToHex(gas_limit_));
  dict.Set("to", to_.ToHex());
  dict.Set("value", Uint256ValueToHex(value_));
  dict.Set("data", base::Base64Encode(data_));
  dict.Set("v", static_cast<int>(v_));
  dict.Set("r", base::Base64Encode(r_));
  dict.Set("s", base::Base64Encode(s_));
  dict.Set("type", static_cast<int>(type_));

  return dict;
}

uint256_t EthTransaction::GetBaseFee() const {
  uint256_t fee = GetDataFee() + kTransactionCost;
  if (IsToCreationAddress()) {
    fee += kContractCreationCost;
  }

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

base::Value EthTransaction::Serialize() const {
  base::Value::List list;
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(gas_price_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(to_.bytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(RLPUint256ToBlob(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  return base::Value(std::move(list));
}

}  // namespace brave_wallet
