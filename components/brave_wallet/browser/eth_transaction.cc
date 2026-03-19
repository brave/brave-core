/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/logging.h"
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

EthTransaction::EthTransaction() = default;
EthTransaction::EthTransaction(const EthTransaction&) = default;
EthTransaction::EthTransaction(
    uint256_t chain_id,
    std::optional<uint256_t> nonce,
    uint256_t gas_price,
    uint256_t gas_limit,
    std::variant<EthAddress, EthContractCreationAddress> to,
    uint256_t value,
    const std::vector<uint8_t>& data)
    : chain_id_(chain_id),
      nonce_(nonce),
      gas_price_(gas_price),
      gas_limit_(gas_limit),
      to_(to),
      value_(value),
      data_(data) {}
EthTransaction::~EthTransaction() = default;

// static
std::optional<EthTransaction> EthTransaction::FromTxData(
    const mojom::TxDataPtr& tx_data,
    bool strict) {
  EthTransaction tx;
  if (!HexValueToUint256(tx_data->chain_id, &tx.chain_id_)) {
    return std::nullopt;
  }

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

  if (tx_data->to.empty() ||
      tx_data->to == EthContractCreationAddress().ToHex()) {
    tx.to_ = EthContractCreationAddress();
  } else {
    auto addr = EthAddress::From0xHex(tx_data->to);
    if (!addr) {
      return std::nullopt;
    }
    tx.to_ = *addr;
  }
  if (!HexValueToUint256(tx_data->value, &tx.value_) && strict) {
    return std::nullopt;
  }
  tx.data_ = tx_data->data;
  return tx;
}

// static
std::optional<EthTransaction> EthTransaction::FromValue(
    const base::DictValue& value) {
  EthTransaction tx;

  const std::string* tx_chain_id = value.FindString("chain_id");
  if (!tx_chain_id) {
    return std::nullopt;
  }
  if (!HexValueToUint256(*tx_chain_id, &tx.chain_id_)) {
    return std::nullopt;
  }

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

  if (to->empty() || *to == EthContractCreationAddress().ToHex()) {
    tx.set_to(EthContractCreationAddress());
  } else {
    auto addr = EthAddress::From0xHex(*to);
    if (!addr) {
      return std::nullopt;
    }
    tx.set_to(*addr);
  }

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
  tx.type_ = static_cast<EthTransactionType>(*type);
  if (tx.type_ != EthTransactionType::kLegacy &&
      tx.type_ != EthTransactionType::kEip2930 &&
      tx.type_ != EthTransactionType::kEip1559) {
    return std::nullopt;
  }
  return tx;
}

std::vector<uint8_t> EthTransaction::GetMessageToSign() const {
  return GetMessageToSignImpl();
}

std::vector<uint8_t> EthTransaction::GetMessageToSignImpl() const {
  DCHECK(nonce_);
  base::ListValue list;
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(gas_price_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(GetToBytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  if (chain_id_) {
    list.Append(RLPUint256ToBlob(chain_id_));
    list.Append(RLPUint256ToBlob(0));
    list.Append(RLPUint256ToBlob(0));
  }

  return RLPEncode(list);
}

KeccakHashArray EthTransaction::GetHashedMessageToSign() const {
  return KeccakHash(GetMessageToSign());
}

std::string EthTransaction::GetSignedTransaction() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(Serialize());
}

std::string EthTransaction::GetTransactionHash() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(KeccakHash(Serialize()));
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
void EthTransaction::ProcessSignature(const Secp256k1Signature& signature) {
  r_ = base::ToVector(signature.rs_bytes().subspan(0u, 32u));
  s_ = base::ToVector(signature.rs_bytes().subspan(32u, 32u));

  if (type_ != EthTransactionType::kLegacy) {
    // For EIP-1559 and EIP-2930 recovery id is used as is.
    v_ = signature.recid();
  } else {
    // For EIP-155 recovery id is adjusted with chain_id.
    v_ = chain_id_ ? static_cast<uint256_t>(signature.recid()) +
                         (chain_id_ * static_cast<uint256_t>(2) +
                          static_cast<uint256_t>(35))
                   : static_cast<uint256_t>(signature.recid()) +
                         static_cast<uint256_t>(27);
  }
}

bool EthTransaction::IsSigned() const {
  return !r_.empty() && !s_.empty();
}

base::DictValue EthTransaction::ToValue() const {
  return ToValueImpl();
}

base::DictValue EthTransaction::ToValueImpl() const {
  base::DictValue dict;
  dict.Set("chain_id", Uint256ValueToHex(chain_id_));
  dict.Set("nonce", nonce_ ? Uint256ValueToHex(nonce_.value()) : "");
  dict.Set("gas_price", Uint256ValueToHex(gas_price_));
  dict.Set("gas_limit", Uint256ValueToHex(gas_limit_));
  dict.Set("to", GetToHex());
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

std::vector<uint8_t> EthTransaction::Serialize() const {
  base::ListValue list;
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(gas_price_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(GetToBytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(RLPUint256ToBlob(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  return RLPEncode(base::Value(std::move(list)));
}

std::vector<uint8_t> EthTransaction::GetToBytes() const {
  auto* contract_creation = std::get_if<EthContractCreationAddress>(&to_);
  auto* eth_addr = std::get_if<EthAddress>(&to_);
  CHECK(contract_creation || eth_addr);
  if (contract_creation) {
    return base::ToVector(contract_creation->bytes());
  } else {
    return base::ToVector(eth_addr->bytes());
  }
}

std::string EthTransaction::GetToHex() const {
  auto* contract_creation = std::get_if<EthContractCreationAddress>(&to_);
  auto* eth_addr = std::get_if<EthAddress>(&to_);
  CHECK(contract_creation || eth_addr);
  if (contract_creation) {
    return contract_creation->ToHex();
  } else {
    return eth_addr->ToHex();
  }
}

std::string EthTransaction::GetToChecksumAddress() const {
  auto* contract_creation = std::get_if<EthContractCreationAddress>(&to_);
  auto* eth_addr = std::get_if<EthAddress>(&to_);
  CHECK(contract_creation || eth_addr);
  if (contract_creation) {
    return contract_creation->ToHex();
  } else {
    return eth_addr->ToChecksumAddress();
  }
}

}  // namespace brave_wallet
