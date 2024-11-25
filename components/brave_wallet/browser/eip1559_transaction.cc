/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip1559_transaction.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/containers/to_vector.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

// static
std::optional<Eip1559Transaction::GasEstimation>
Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
    mojom::GasEstimation1559Ptr gas_estimation) {
  if (!gas_estimation) {
    return std::nullopt;
  }

  GasEstimation estimation;
  if (!HexValueToUint256(gas_estimation->slow_max_priority_fee_per_gas,
                         &estimation.slow_max_priority_fee_per_gas)) {
    return std::nullopt;
  }
  if (!HexValueToUint256(gas_estimation->avg_max_priority_fee_per_gas,
                         &estimation.avg_max_priority_fee_per_gas)) {
    return std::nullopt;
  }
  if (!HexValueToUint256(gas_estimation->fast_max_priority_fee_per_gas,
                         &estimation.fast_max_priority_fee_per_gas)) {
    return std::nullopt;
  }
  if (!HexValueToUint256(gas_estimation->slow_max_fee_per_gas,
                         &estimation.slow_max_fee_per_gas)) {
    return std::nullopt;
  }
  if (!HexValueToUint256(gas_estimation->avg_max_fee_per_gas,
                         &estimation.avg_max_fee_per_gas)) {
    return std::nullopt;
  }
  if (!HexValueToUint256(gas_estimation->fast_max_fee_per_gas,
                         &estimation.fast_max_fee_per_gas)) {
    return std::nullopt;
  }
  if (!HexValueToUint256(gas_estimation->base_fee_per_gas,
                         &estimation.base_fee_per_gas)) {
    return std::nullopt;
  }

  return estimation;
}

// static
mojom::GasEstimation1559Ptr
Eip1559Transaction::GasEstimation::ToMojomGasEstimation1559(
    Eip1559Transaction::GasEstimation gas_estimation) {
  mojom::GasEstimation1559Ptr estimation = mojom::GasEstimation1559::New();
  estimation->slow_max_priority_fee_per_gas =
      Uint256ValueToHex(gas_estimation.slow_max_priority_fee_per_gas);
  estimation->avg_max_priority_fee_per_gas =
      Uint256ValueToHex(gas_estimation.avg_max_priority_fee_per_gas);
  estimation->fast_max_priority_fee_per_gas =
      Uint256ValueToHex(gas_estimation.fast_max_priority_fee_per_gas);
  estimation->slow_max_fee_per_gas =
      Uint256ValueToHex(gas_estimation.slow_max_fee_per_gas);
  estimation->avg_max_fee_per_gas =
      Uint256ValueToHex(gas_estimation.avg_max_fee_per_gas);
  estimation->fast_max_fee_per_gas =
      Uint256ValueToHex(gas_estimation.fast_max_fee_per_gas);
  estimation->base_fee_per_gas =
      Uint256ValueToHex(gas_estimation.base_fee_per_gas);
  return estimation;
}

bool Eip1559Transaction::GasEstimation::operator==(
    const Eip1559Transaction::GasEstimation& estimation) const {
  return slow_max_priority_fee_per_gas ==
             estimation.slow_max_priority_fee_per_gas &&
         avg_max_priority_fee_per_gas ==
             estimation.avg_max_priority_fee_per_gas &&
         fast_max_priority_fee_per_gas ==
             estimation.fast_max_priority_fee_per_gas &&
         slow_max_fee_per_gas == estimation.slow_max_fee_per_gas &&
         avg_max_fee_per_gas == estimation.avg_max_fee_per_gas &&
         fast_max_fee_per_gas == estimation.fast_max_fee_per_gas &&
         base_fee_per_gas == estimation.base_fee_per_gas;
}

Eip1559Transaction::Eip1559Transaction()
    : max_priority_fee_per_gas_(0), max_fee_per_gas_(0) {
  type_ = 2;
}

Eip1559Transaction::Eip1559Transaction(std::optional<uint256_t> nonce,
                                       uint256_t gas_price,
                                       uint256_t gas_limit,
                                       const EthAddress& to,
                                       uint256_t value,
                                       const std::vector<uint8_t>& data,
                                       uint256_t chain_id,
                                       uint256_t max_priority_fee_per_gas,
                                       uint256_t max_fee_per_gas,
                                       GasEstimation gas_estimation)
    : Eip2930Transaction(nonce,
                         gas_price,
                         gas_limit,
                         to,
                         value,
                         data,
                         chain_id),
      max_priority_fee_per_gas_(max_priority_fee_per_gas),
      max_fee_per_gas_(max_fee_per_gas),
      gas_estimation_(gas_estimation) {
  type_ = 2;
}
Eip1559Transaction::Eip1559Transaction(const Eip1559Transaction&) = default;
Eip1559Transaction::~Eip1559Transaction() = default;

bool Eip1559Transaction::operator==(const Eip1559Transaction& tx) const {
  return Eip2930Transaction::operator==(tx) &&
         max_priority_fee_per_gas_ == tx.max_priority_fee_per_gas_ &&
         max_fee_per_gas_ == tx.max_fee_per_gas_;
}

// static
std::optional<Eip1559Transaction> Eip1559Transaction::FromTxData(
    const mojom::TxData1559Ptr& tx_data1559,
    bool strict) {
  uint256_t chain_id = 0;
  if (!HexValueToUint256(tx_data1559->chain_id, &chain_id) && strict) {
    return std::nullopt;
  }

  std::optional<Eip2930Transaction> tx_2930 =
      Eip2930Transaction::FromTxData(tx_data1559->base_data, chain_id, strict);
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

  GasEstimation gas_estimation;
  auto estimation = GasEstimation::FromMojomGasEstimation1559(
      std::move(tx_data1559->gas_estimation));
  if (estimation) {
    gas_estimation = estimation.value();
  }

  Eip1559Transaction tx(
      tx_2930->nonce(), tx_2930->gas_price(), tx_2930->gas_limit(),
      tx_2930->to(), tx_2930->value(), tx_2930->data(), tx_2930->chain_id(),
      max_priority_fee_per_gas, max_fee_per_gas, gas_estimation);
  return tx;
}

// static
std::optional<Eip1559Transaction> Eip1559Transaction::FromValue(
    const base::Value::Dict& value) {
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

  GasEstimation estimation;
  const base::Value::Dict* estimation_dict = value.FindDict("gas_estimation");
  if (estimation_dict) {
    const std::string* tx_slow_max_priority_fee_per_gas =
        estimation_dict->FindString("slow_max_priority_fee_per_gas");
    if (!tx_slow_max_priority_fee_per_gas ||
        !HexValueToUint256(*tx_slow_max_priority_fee_per_gas,
                           &estimation.slow_max_priority_fee_per_gas)) {
      return std::nullopt;
    }

    const std::string* tx_avg_max_priority_fee_per_gas =
        estimation_dict->FindString("avg_max_priority_fee_per_gas");
    if (!tx_avg_max_priority_fee_per_gas ||
        !HexValueToUint256(*tx_avg_max_priority_fee_per_gas,
                           &estimation.avg_max_priority_fee_per_gas)) {
      return std::nullopt;
    }

    const std::string* tx_fast_max_priority_fee_per_gas =
        estimation_dict->FindString("fast_max_priority_fee_per_gas");
    if (!tx_fast_max_priority_fee_per_gas ||
        !HexValueToUint256(*tx_fast_max_priority_fee_per_gas,
                           &estimation.fast_max_priority_fee_per_gas)) {
      return std::nullopt;
    }

    const std::string* tx_slow_max_fee_per_gas =
        estimation_dict->FindString("slow_max_fee_per_gas");
    if (!tx_slow_max_fee_per_gas ||
        !HexValueToUint256(*tx_slow_max_fee_per_gas,
                           &estimation.slow_max_fee_per_gas)) {
      return std::nullopt;
    }

    const std::string* tx_avg_max_fee_per_gas =
        estimation_dict->FindString("avg_max_fee_per_gas");
    if (!tx_avg_max_fee_per_gas ||
        !HexValueToUint256(*tx_avg_max_fee_per_gas,
                           &estimation.avg_max_fee_per_gas)) {
      return std::nullopt;
    }

    const std::string* tx_fast_max_fee_per_gas =
        estimation_dict->FindString("fast_max_fee_per_gas");
    if (!tx_fast_max_fee_per_gas ||
        !HexValueToUint256(*tx_fast_max_fee_per_gas,
                           &estimation.fast_max_fee_per_gas)) {
      return std::nullopt;
    }

    const std::string* tx_base_fee_per_gas =
        estimation_dict->FindString("base_fee_per_gas");
    if (!tx_base_fee_per_gas ||
        !HexValueToUint256(*tx_base_fee_per_gas,
                           &estimation.base_fee_per_gas)) {
      return std::nullopt;
    }
  }

  Eip1559Transaction tx(tx_2930->nonce(), tx_2930->gas_price(),
                        tx_2930->gas_limit(), tx_2930->to(), tx_2930->value(),
                        tx_2930->data(), tx_2930->chain_id(),
                        max_priority_fee_per_gas, max_fee_per_gas, estimation);
  tx.v_ = tx_2930->v();
  tx.r_ = tx_2930->r();
  tx.s_ = tx_2930->s();

  return tx;
}

std::vector<uint8_t> Eip1559Transaction::GetMessageToSign(uint256_t chain_id,
                                                          bool hash) const {
  DCHECK(nonce_);
  std::vector<uint8_t> result;
  result.push_back(type_);

  base::Value::List list;
  list.Append(RLPUint256ToBlob(chain_id_));
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(max_priority_fee_per_gas_));
  list.Append(RLPUint256ToBlob(max_fee_per_gas_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(to_.bytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));

  const std::string rlp_msg = RLPEncode(base::Value(std::move(list)));
  result.insert(result.end(), rlp_msg.begin(), rlp_msg.end());
  return hash ? base::ToVector(KeccakHash(result)) : result;
}

std::string Eip1559Transaction::GetSignedTransaction() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(Serialize());
}

std::string Eip1559Transaction::GetTransactionHash() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(KeccakHash(Serialize()));
}

base::Value::Dict Eip1559Transaction::ToValue() const {
  base::Value::Dict tx = Eip2930Transaction::ToValue();

  tx.Set("max_priority_fee_per_gas",
         Uint256ValueToHex(max_priority_fee_per_gas_));
  tx.Set("max_fee_per_gas", Uint256ValueToHex(max_fee_per_gas_));

  base::Value::Dict& estimation =
      tx.Set("gas_estimation", base::Value::Dict())->GetDict();
  estimation.Set(
      "slow_max_priority_fee_per_gas",
      Uint256ValueToHex(gas_estimation_.slow_max_priority_fee_per_gas));
  estimation.Set(
      "avg_max_priority_fee_per_gas",
      Uint256ValueToHex(gas_estimation_.avg_max_priority_fee_per_gas));
  estimation.Set(
      "fast_max_priority_fee_per_gas",
      Uint256ValueToHex(gas_estimation_.fast_max_priority_fee_per_gas));
  estimation.Set("slow_max_fee_per_gas",
                 Uint256ValueToHex(gas_estimation_.slow_max_fee_per_gas));
  estimation.Set("avg_max_fee_per_gas",
                 Uint256ValueToHex(gas_estimation_.avg_max_fee_per_gas));
  estimation.Set("fast_max_fee_per_gas",
                 Uint256ValueToHex(gas_estimation_.fast_max_fee_per_gas));
  estimation.Set("base_fee_per_gas",
                 Uint256ValueToHex(gas_estimation_.base_fee_per_gas));

  return tx;
}

uint256_t Eip1559Transaction::GetUpfrontCost(uint256_t block_base_fee) const {
  uint256_t inclusion_fee_per_gas =
      std::min(max_priority_fee_per_gas_, max_fee_per_gas_ - block_base_fee);
  uint256_t gas_price = inclusion_fee_per_gas + block_base_fee;

  return gas_limit_ * gas_price + value_;
}

std::vector<uint8_t> Eip1559Transaction::Serialize() const {
  base::Value::List list;
  list.Append(RLPUint256ToBlob(chain_id_));
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(max_priority_fee_per_gas_));
  list.Append(RLPUint256ToBlob(max_fee_per_gas_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(to_.bytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));
  list.Append(RLPUint256ToBlob(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  std::vector<uint8_t> result;
  result.push_back(type_);

  const std::string rlp_msg = RLPEncode(base::Value(std::move(list)));
  result.insert(result.end(), rlp_msg.begin(), rlp_msg.end());

  return result;
}

}  // namespace brave_wallet
