/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <utility>

#include "base/base64.h"
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
EthTransaction::EthTransaction(absl::optional<uint256_t> nonce,
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
    const mojom::TxDataPtr& tx_data,
    bool strict) {
  EthTransaction tx;
  if (!tx_data->nonce.empty()) {
    uint256_t nonce_uint;
    if (HexValueToUint256(tx_data->nonce, &nonce_uint)) {
      tx.nonce_ = nonce_uint;
    } else if (strict) {
      return absl::nullopt;
    }
  }

  if (!HexValueToUint256(tx_data->gas_price, &tx.gas_price_) && strict)
    return absl::nullopt;
  if (!HexValueToUint256(tx_data->gas_limit, &tx.gas_limit_) && strict)
    return absl::nullopt;
  tx.to_ = EthAddress::FromHex(tx_data->to);
  if (!HexValueToUint256(tx_data->value, &tx.value_) && strict)
    return absl::nullopt;
  tx.data_ = tx_data->data;
  return tx;
}

// static
absl::optional<EthTransaction> EthTransaction::FromValue(
    const base::Value::Dict& value) {
  EthTransaction tx;
  const std::string* nonce = value.FindString("nonce");
  if (!nonce)
    return absl::nullopt;

  if (!nonce->empty()) {
    uint256_t nonce_uint;
    if (!HexValueToUint256(*nonce, &nonce_uint))
      return absl::nullopt;
    tx.nonce_ = nonce_uint;
  }

  const std::string* gas_price = value.FindString("gas_price");
  if (!gas_price)
    return absl::nullopt;
  if (!HexValueToUint256(*gas_price, &tx.gas_price_))
    return absl::nullopt;

  const std::string* gas_limit = value.FindString("gas_limit");
  if (!gas_limit)
    return absl::nullopt;
  if (!HexValueToUint256(*gas_limit, &tx.gas_limit_))
    return absl::nullopt;

  const std::string* to = value.FindString("to");
  if (!to)
    return absl::nullopt;
  tx.to_ = EthAddress::FromHex(*to);

  const std::string* tx_value = value.FindString("value");
  if (!tx_value)
    return absl::nullopt;
  if (!HexValueToUint256(*tx_value, &tx.value_))
    return absl::nullopt;

  const std::string* data = value.FindString("data");
  if (!data)
    return absl::nullopt;
  std::string data_decoded;
  if (!base::Base64Decode(*data, &data_decoded))
    return absl::nullopt;
  tx.data_ = std::vector<uint8_t>(data_decoded.begin(), data_decoded.end());

  absl::optional<int> v = value.FindInt("v");
  if (!v)
    return absl::nullopt;
  tx.v_ = (uint8_t)*v;

  const std::string* r = value.FindString("r");
  if (!r)
    return absl::nullopt;
  std::string r_decoded;
  if (!base::Base64Decode(*r, &r_decoded))
    return absl::nullopt;
  tx.r_ = std::vector<uint8_t>(r_decoded.begin(), r_decoded.end());

  const std::string* s = value.FindString("s");
  if (!s)
    return absl::nullopt;
  std::string s_decoded;
  if (!base::Base64Decode(*s, &s_decoded))
    return absl::nullopt;
  tx.s_ = std::vector<uint8_t>(s_decoded.begin(), s_decoded.end());

  absl::optional<int> type = value.FindInt("type");
  if (!type)
    return absl::nullopt;
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

  const std::string message = RLPEncode(base::Value(std::move(list)));
  auto result = std::vector<uint8_t>(message.begin(), message.end());
  return hash ? KeccakHash(result) : result;
}

std::string EthTransaction::GetSignedTransaction() const {
  DCHECK(nonce_);
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

  return ToHex(RLPEncode(base::Value(std::move(list))));
}

bool EthTransaction::ProcessVRS(const std::string& v,
                                const std::string& r,
                                const std::string& s) {
  if (!base::StartsWith(v, "0x") || !base::StartsWith(r, "0x") ||
      !base::StartsWith(s, "0x"))
    return false;
  uint256_t v_decoded;
  if (!HexValueToUint256(v, &v_decoded)) {
    LOG(ERROR) << "Unable to decode v param";
    return false;
  }

  std::vector<uint8_t> r_decoded;
  if (!PrefixedHexStringToBytes(r, &r_decoded)) {
    LOG(ERROR) << "Unable to decode r param";
    return false;
  }
  std::vector<uint8_t> s_decoded;
  if (!PrefixedHexStringToBytes(s, &s_decoded)) {
    LOG(ERROR) << "Unable to decode s param";
    return false;
  }

  r_ = r_decoded;
  s_ = s_decoded;
  v_ = v_decoded;
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
