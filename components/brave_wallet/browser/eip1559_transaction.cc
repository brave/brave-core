/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

Eip1559Transaction::Eip1559Transaction() {
  type_ = EthTransactionType::kEip1559;
}

Eip1559Transaction::Eip1559Transaction(
    uint256_t chain_id,
    std::optional<uint256_t> nonce,
    uint256_t gas_price,
    uint256_t gas_limit,
    const std::variant<EthAddress, EthContractCreationAddress>& to,
    uint256_t value,
    const std::vector<uint8_t>& data,
    uint256_t max_priority_fee_per_gas,
    uint256_t max_fee_per_gas)
    : Eip2930Transaction(chain_id,
                         nonce,
                         gas_price,
                         gas_limit,
                         to,
                         value,
                         data),
      max_priority_fee_per_gas_(max_priority_fee_per_gas),
      max_fee_per_gas_(max_fee_per_gas) {
  type_ = EthTransactionType::kEip1559;
}

Eip1559Transaction::Eip1559Transaction(const Eip1559Transaction&) = default;
Eip1559Transaction::~Eip1559Transaction() = default;

// static
std::optional<Eip1559Transaction> Eip1559Transaction::FromTxData(
    const mojom::TxData1559Ptr& tx_data1559,
    bool strict) {
  std::optional<Eip2930Transaction> tx_2930 =
      Eip2930Transaction::FromTxData(tx_data1559->base_data, strict);
  if (!tx_2930) {
    return std::nullopt;
  }

  uint256_t max_priority_fee_per_gas = 0;
  if (!HexValueToUint256(tx_data1559->max_priority_fee_per_gas,
                         &max_priority_fee_per_gas) &&
      strict) {
    return std::nullopt;
  }
  uint256_t max_fee_per_gas = 0;
  if (!HexValueToUint256(tx_data1559->max_fee_per_gas, &max_fee_per_gas) &&
      strict) {
    return std::nullopt;
  }

  return Eip1559Transaction(tx_2930->chain_id(), tx_2930->nonce(),
                            tx_2930->gas_price(), tx_2930->gas_limit(),
                            tx_2930->to(), tx_2930->value(), tx_2930->data(),
                            max_priority_fee_per_gas, max_fee_per_gas);
}

// static
std::optional<Eip1559Transaction> Eip1559Transaction::FromValue(
    const base::DictValue& value) {
  std::optional<Eip2930Transaction> tx_2930 =
      Eip2930Transaction::FromValue(value);
  if (!tx_2930) {
    return std::nullopt;
  }

  const std::string* tx_max_priority_fee_per_gas =
      value.FindString("max_priority_fee_per_gas");
  if (!tx_max_priority_fee_per_gas) {
    return std::nullopt;
  }
  uint256_t max_priority_fee_per_gas;
  if (!HexValueToUint256(*tx_max_priority_fee_per_gas,
                         &max_priority_fee_per_gas)) {
    return std::nullopt;
  }

  const std::string* tx_max_fee_per_gas = value.FindString("max_fee_per_gas");
  if (!tx_max_fee_per_gas) {
    return std::nullopt;
  }
  uint256_t max_fee_per_gas;
  if (!HexValueToUint256(*tx_max_fee_per_gas, &max_fee_per_gas)) {
    return std::nullopt;
  }

  Eip1559Transaction tx(tx_2930->chain_id(), tx_2930->nonce(),
                        tx_2930->gas_price(), tx_2930->gas_limit(),
                        tx_2930->to(), tx_2930->value(), tx_2930->data(),
                        max_priority_fee_per_gas, max_fee_per_gas);
  tx.v_ = tx_2930->v();
  tx.r_ = tx_2930->r();
  tx.s_ = tx_2930->s();
  tx.access_list_ = std::move(*tx_2930->access_list());

  return tx;
}

std::vector<uint8_t> Eip1559Transaction::GetMessageToSignImpl() const {
  DCHECK(chain_id_);
  DCHECK(nonce_);

  base::ListValue list;
  list.Append(RLPUint256ToBlob(chain_id_));
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(max_priority_fee_per_gas_));
  list.Append(RLPUint256ToBlob(max_fee_per_gas_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(GetToBytes());

  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));

  std::vector<uint8_t> result;
  result.push_back(static_cast<uint8_t>(type_));
  base::Extend(result, RLPEncode(list));
  return result;
}

base::DictValue Eip1559Transaction::ToValueImpl() const {
  base::DictValue tx = Eip2930Transaction::ToValueImpl();

  tx.Set("max_priority_fee_per_gas",
         Uint256ValueToHex(max_priority_fee_per_gas_));
  tx.Set("max_fee_per_gas", Uint256ValueToHex(max_fee_per_gas_));

  return tx;
}

std::vector<uint8_t> Eip1559Transaction::Serialize() const {
  DCHECK(chain_id_);

  base::ListValue list;
  list.Append(RLPUint256ToBlob(chain_id_));
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(max_priority_fee_per_gas_));
  list.Append(RLPUint256ToBlob(max_fee_per_gas_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(GetToBytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));
  list.Append(RLPUint256ToBlob(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  std::vector<uint8_t> result;
  result.push_back(static_cast<uint8_t>(type_));

  base::Extend(result, RLPEncode(list));

  return result;
}

}  // namespace brave_wallet
