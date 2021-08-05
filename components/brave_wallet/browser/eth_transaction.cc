/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_transaction.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/logging.h"
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
EthTransaction::EthTransaction(mojom::TxDataPtr tx_data)
    : nonce_(tx_data->nonce),
      gas_price_(tx_data->gas_price),
      gas_limit_(tx_data->gas_limit),
      to_(tx_data->to),
      value_(tx_data->value),
      data_(tx_data->data) {}

EthTransaction::~EthTransaction() = default;

bool EthTransaction::operator==(const EthTransaction& tx) const {
  return nonce_ == tx.nonce_ && gas_price_ == tx.gas_price_ &&
         gas_limit_ == tx.gas_limit_ && to_ == tx.to_ && value_ == tx.value_ &&
         std::equal(data_.begin(), data_.end(), tx.data_.begin()) &&
         v_ == tx.v_ && std::equal(r_.begin(), r_.end(), tx.r_.begin()) &&
         std::equal(s_.begin(), s_.end(), tx.s_.begin()) && type_ == tx.type_;
}

mojo::PendingRemote<mojom::EthTransaction> EthTransaction::MakeRemote() {
  mojo::PendingRemote<mojom::EthTransaction> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

// static
std::unique_ptr<EthTransaction> EthTransaction::FromValue(
    const base::Value& value) {
  auto tx = std::make_unique<EthTransaction>();
  const std::string* nonce = value.FindStringKey("nonce");
  if (!nonce)
    return nullptr;
  tx->nonce_ = *nonce;

  const std::string* gas_price = value.FindStringKey("gas_price");
  if (!gas_price)
    return nullptr;
  tx->gas_price_ = *gas_price;

  const std::string* gas_limit = value.FindStringKey("gas_limit");
  if (!gas_limit)
    return nullptr;
  tx->gas_limit_ = *gas_limit;

  const std::string* to = value.FindStringKey("to");
  if (!to)
    return nullptr;
  tx->to_ = *to;

  const std::string* tx_value = value.FindStringKey("value");
  if (!tx_value)
    return nullptr;
  tx->value_ = *tx_value;

  const std::string* data = value.FindStringKey("data");
  if (!data)
    return nullptr;
  std::string data_decoded;
  if (!base::Base64Decode(*data, &data_decoded))
    return nullptr;
  tx->data_ = std::vector<uint8_t>(data_decoded.begin(), data_decoded.end());

  absl::optional<int> v = value.FindIntKey("v");
  if (!v)
    return nullptr;
  tx->v_ = (uint8_t)*v;

  const std::string* r = value.FindStringKey("r");
  if (!r)
    return nullptr;
  std::string r_decoded;
  if (!base::Base64Decode(*r, &r_decoded))
    return nullptr;
  tx->r_ = std::vector<uint8_t>(r_decoded.begin(), r_decoded.end());

  const std::string* s = value.FindStringKey("s");
  if (!s)
    return nullptr;
  std::string s_decoded;
  if (!base::Base64Decode(*s, &s_decoded))
    return nullptr;
  tx->s_ = std::vector<uint8_t>(s_decoded.begin(), s_decoded.end());

  absl::optional<int> type = value.FindIntKey("type");
  if (!type)
    return nullptr;
  tx->type_ = (uint32_t)*type;

  return tx;
}

bool EthTransaction::GetBasicListData(base::ListValue* list) const {
  CHECK(list);
  uint256_t nonce_uint = 0;
  if (!HexValueToUint256(nonce_, &nonce_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(nonce_uint));

  uint256_t gas_price_uint = 0;
  if (!HexValueToUint256(gas_price_, &gas_price_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(gas_price_uint));

  uint256_t gas_limit_uint = 0;
  if (!HexValueToUint256(gas_limit_, &gas_limit_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(gas_limit_uint));

  EthAddress to_address = EthAddress::FromHex(to_);
  if (to_address.IsEmpty()) {
    return false;
  }
  list->Append(base::Value(to_address.bytes()));

  uint256_t value_uint = 0;
  if (!HexValueToUint256(value_, &value_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(value_uint));
  list->Append(base::Value(data_));
  return true;
}

bool EthTransaction::GetSignatureListData(base::ListValue* list) const {
  CHECK(list);
  list->Append(base::Value(v_));
  list->Append(base::Value(r_));
  list->Append(base::Value(s_));
  return true;
}

void EthTransaction::GetMessageToSign(const std::string& chain_id,
                                      GetMessageToSignCallback callback) {
  base::ListValue list;
  if (!GetBasicListData(&list)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  uint256_t chain_id_uint = 0;
  if (!HexValueToUint256(chain_id, &chain_id_uint)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }
  if (chain_id_uint) {
    list.Append(RLPUint256ToBlobValue(chain_id_uint));
    list.Append(RLPUint256ToBlobValue(0));
    list.Append(RLPUint256ToBlobValue(0));
  }

  const std::string message = RLPEncode(std::move(list));
  std::move(callback).Run(
      true, KeccakHash(std::vector<uint8_t>(message.begin(), message.end())));
}

void EthTransaction::GetSignedTransaction(
    GetSignedTransactionCallback callback) {
  base::ListValue list;
  if (!GetBasicListData(&list) || !GetSignatureListData(&list)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, ToHex(RLPEncode(std::move(list))));
}

// signature and recid will be used to produce v, r, s
void EthTransaction::ProcessSignature(const std::vector<uint8_t>& signature,
                                      int recid,
                                      const std::string& chain_id) {
  if (signature.size() != 64) {
    LOG(ERROR) << __func__ << ": signature length should be 64 bytes";
    return;
  }
  if (recid < 0 || recid > 3) {
    LOG(ERROR) << __func__ << ": recovery id must be 0, 1, 2 or 3";
    return;
  }
  uint256_t chain_id_uint = 0;
  if (!HexValueToUint256(chain_id, &chain_id_uint)) {
    LOG(ERROR) << __func__ << ": Could not convert chain id";
    return;
  }

  r_ = std::vector<uint8_t>(signature.begin(),
                            signature.begin() + signature.size() / 2);
  s_ = std::vector<uint8_t>(signature.begin() + signature.size() / 2,
                            signature.end());
  v_ = chain_id_uint
           ? static_cast<uint256_t>(recid) +
                 (chain_id_uint * static_cast<uint256_t>(2) +
                  static_cast<uint256_t>(35))
           : static_cast<uint256_t>(recid) + static_cast<uint256_t>(27);
}

bool EthTransaction::IsSigned() const {
  return v_ != 0 && r_.size() != 0 && s_.size() != 0;
}

base::Value EthTransaction::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("nonce", nonce_);
  dict.SetStringKey("gas_price", gas_price_);
  dict.SetStringKey("gas_limit", gas_limit_);
  dict.SetStringKey("to", to_);
  dict.SetStringKey("value", value_);
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
  uint256_t gas_price_uint = 0;
  if (!HexValueToUint256(gas_price_, &gas_price_uint)) {
    return 0;
  }
  uint256_t gas_limit_uint = 0;
  if (!HexValueToUint256(gas_limit_, &gas_limit_uint)) {
    return 0;
  }
  uint256_t value_uint = 0;
  if (!HexValueToUint256(value_, &value_uint)) {
    return 0;
  }

  return gas_limit_uint * gas_price_uint + value_uint;
}

}  // namespace brave_wallet
