/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"

namespace brave_wallet {

Eip1559Transaction::Eip1559Transaction() = default;
Eip1559Transaction::Eip1559Transaction(
    mojom::TxDataPtr tx_data,
    const std::string& chain_id,
    const std::string& max_priority_fee_per_gas,
    const std::string& max_fee_per_gas)
    : Eip2930Transaction(std::move(tx_data), chain_id),
      max_priority_fee_per_gas_(max_priority_fee_per_gas),
      max_fee_per_gas_(max_fee_per_gas) {
  type_ = 2;
}

Eip1559Transaction::~Eip1559Transaction() = default;

bool Eip1559Transaction::operator==(const Eip1559Transaction& tx) const {
  return Eip2930Transaction::operator==(tx) &&
         max_priority_fee_per_gas_ == tx.max_priority_fee_per_gas_ &&
         max_fee_per_gas_ == tx.max_fee_per_gas_;
}

// static
std::unique_ptr<Eip1559Transaction> Eip1559Transaction::FromValue(
    const base::Value& value) {
  auto tx_2930 = Eip2930Transaction::FromValue(value);
  if (!tx_2930)
    return nullptr;
  auto tx_data = mojom::TxData::New(tx_2930->nonce(), tx_2930->gas_price(),
                                    tx_2930->gas_limit(), tx_2930->to(),
                                    tx_2930->value(), tx_2930->data());

  const std::string* tx_max_priority_fee_per_gas =
      value.FindStringKey("max_priority_fee_per_gas");
  if (!tx_max_priority_fee_per_gas)
    return nullptr;

  const std::string* tx_max_fee_per_gas =
      value.FindStringKey("max_fee_per_gas");
  if (!tx_max_fee_per_gas)
    return nullptr;

  auto tx = std::make_unique<Eip1559Transaction>(
      std::move(tx_data), tx_2930->chain_id(), *tx_max_priority_fee_per_gas,
      *tx_max_fee_per_gas);
  tx->v_ = tx_2930->v();
  tx->r_ = tx_2930->r();
  tx->s_ = tx_2930->s();

  return tx;
}

bool Eip1559Transaction::GetBasicListData(base::ListValue* list) const {
  CHECK(list);
  uint256_t chain_id_uint = 0;
  if (!HexValueToUint256(chain_id_, &chain_id_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(chain_id_uint));

  uint256_t nonce_uint = 0;
  if (!HexValueToUint256(nonce_, &nonce_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(nonce_uint));

  uint256_t max_priority_fee_per_gas_uint = 0;
  if (!HexValueToUint256(max_priority_fee_per_gas_,
                         &max_priority_fee_per_gas_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(max_priority_fee_per_gas_uint));
  uint256_t max_fee_per_gas_uint = 0;
  if (!HexValueToUint256(max_fee_per_gas_, &max_fee_per_gas_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(max_fee_per_gas_uint));

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
  list->Append(base::Value(AccessListToValue(access_list_)));
  return true;
}

void Eip1559Transaction::GetMessageToSign(const std::string& chain_id,
                                          GetMessageToSignCallback callback) {
  std::vector<uint8_t> result;
  result.push_back(type_);

  // TODO(darkdh): Migrate to std::vector<base::Value>, base::ListValue is
  // deprecated
  base::ListValue list;
  if (!GetBasicListData(&list)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  const std::string rlp_msg = RLPEncode(std::move(list));
  result.insert(result.end(), rlp_msg.begin(), rlp_msg.end());
  std::move(callback).Run(true, KeccakHash(result));
}

void Eip1559Transaction::GetSignedTransaction(
    GetSignedTransactionCallback callback) {
  DCHECK(IsSigned());

  // TODO(darkdh): Migrate to std::vector<base::Value>, base::ListValue is
  // deprecated
  base::ListValue list;
  if (!GetBasicListData(&list) || !GetSignatureListData(&list)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::vector<uint8_t> result;
  result.push_back(type_);

  const std::string rlp_msg = RLPEncode(std::move(list));
  result.insert(result.end(), rlp_msg.begin(), rlp_msg.end());

  std::move(callback).Run(true, ToHex(result));
}

base::Value Eip1559Transaction::ToValue() const {
  base::Value tx = Eip2930Transaction::ToValue();

  tx.SetStringKey("max_priority_fee_per_gas", max_priority_fee_per_gas_);
  tx.SetStringKey("max_fee_per_gas", max_fee_per_gas_);

  return tx;
}

uint256_t Eip1559Transaction::GetUpfrontCost(uint256_t block_base_fee) const {
  uint256_t max_priority_fee_per_gas_uint = 0;
  if (!HexValueToUint256(max_priority_fee_per_gas_,
                         &max_priority_fee_per_gas_uint)) {
    return 0;
  }
  uint256_t max_fee_per_gas_uint = 0;
  if (!HexValueToUint256(max_fee_per_gas_, &max_fee_per_gas_uint)) {
    return 0;
  }

  uint256_t inclusion_fee_per_gas = std::min(
      max_priority_fee_per_gas_uint, max_fee_per_gas_uint - block_base_fee);
  uint256_t gas_price = inclusion_fee_per_gas + block_base_fee;

  uint256_t gas_limit_uint = 0;
  if (!HexValueToUint256(gas_limit_, &gas_limit_uint)) {
    return 0;
  }

  uint256_t value_uint = 0;
  if (!HexValueToUint256(value_, &value_uint)) {
    return 0;
  }

  return gas_limit_uint * gas_price + value_uint;
}

}  // namespace brave_wallet
