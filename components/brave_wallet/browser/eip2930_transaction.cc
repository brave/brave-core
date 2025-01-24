/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip2930_transaction.h"

#include <optional>
#include <utility>

#include "base/containers/extend.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {
constexpr uint256_t kAccessListStorageKeyCost = 1900;
constexpr uint256_t kAccessListAddressCost = 2400;
}  // namespace

Eip2930Transaction::AccessListItem::AccessListItem() = default;
Eip2930Transaction::AccessListItem::~AccessListItem() = default;
Eip2930Transaction::AccessListItem::AccessListItem(const AccessListItem&) =
    default;

bool Eip2930Transaction::AccessListItem::operator==(
    const AccessListItem& item) const {
  if (!std::equal(address.begin(), address.end(), item.address.begin())) {
    return false;
  }
  if (storage_keys.size() != item.storage_keys.size()) {
    return false;
  }
  for (size_t i = 0; i < storage_keys.size(); ++i) {
    if (!std::equal(storage_keys[i].begin(), storage_keys[i].end(),
                    item.storage_keys[i].begin())) {
      return false;
    }
  }
  return true;
}

bool Eip2930Transaction::AccessListItem::operator!=(
    const AccessListItem& item) const {
  return !operator==(item);
}

Eip2930Transaction::Eip2930Transaction(const Eip2930Transaction&) = default;
Eip2930Transaction::Eip2930Transaction(std::optional<uint256_t> nonce,
                                       uint256_t gas_price,
                                       uint256_t gas_limit,
                                       const EthAddress& to,
                                       uint256_t value,
                                       const std::vector<uint8_t>& data,
                                       uint256_t chain_id)
    : EthTransaction(nonce, gas_price, gas_limit, to, value, data),
      chain_id_(chain_id) {
  type_ = 1;
}
Eip2930Transaction::Eip2930Transaction() : chain_id_(0) {
  type_ = 1;
}
Eip2930Transaction::~Eip2930Transaction() = default;

bool Eip2930Transaction::operator==(const Eip2930Transaction& tx) const {
  return EthTransaction::operator==(tx) && chain_id_ == tx.chain_id_ &&
         std::equal(access_list_.begin(), access_list_.end(),
                    tx.access_list_.begin());
}

// static
std::optional<Eip2930Transaction> Eip2930Transaction::FromTxData(
    const mojom::TxDataPtr& tx_data,
    uint256_t chain_id,
    bool strict) {
  std::optional<EthTransaction> legacy_tx =
      EthTransaction::FromTxData(tx_data, strict);
  if (!legacy_tx) {
    return std::nullopt;
  }
  return Eip2930Transaction(legacy_tx->nonce(), legacy_tx->gas_price(),
                            legacy_tx->gas_limit(), legacy_tx->to(),
                            legacy_tx->value(), legacy_tx->data(), chain_id);
}

// static
std::optional<Eip2930Transaction> Eip2930Transaction::FromValue(
    const base::Value::Dict& value) {
  std::optional<EthTransaction> legacy_tx = EthTransaction::FromValue(value);
  if (!legacy_tx) {
    return std::nullopt;
  }
  const std::string* tx_chain_id = value.FindString("chain_id");
  if (!tx_chain_id) {
    return std::nullopt;
  }
  uint256_t chain_id;
  if (!HexValueToUint256(*tx_chain_id, &chain_id)) {
    return std::nullopt;
  }

  Eip2930Transaction tx(legacy_tx->nonce(), legacy_tx->gas_price(),
                        legacy_tx->gas_limit(), legacy_tx->to(),
                        legacy_tx->value(), legacy_tx->data(), chain_id);
  tx.v_ = legacy_tx->v();
  tx.r_ = legacy_tx->r();
  tx.s_ = legacy_tx->s();

  const base::Value::List* access_list = value.FindList("access_list");
  if (!access_list) {
    return std::nullopt;
  }
  std::optional<AccessList> access_list_from_value =
      ValueToAccessList(*access_list);
  if (!access_list_from_value) {
    return std::nullopt;
  }
  tx.access_list_ = *access_list_from_value;

  return tx;
}

// static
base::Value::List Eip2930Transaction::AccessListToValue(
    const AccessList& list) {
  base::Value::List access_list;
  for (const AccessListItem& item : list) {
    base::Value::List access_list_item;
    access_list_item.Append(base::Value(item.address));
    base::Value::List storage_keys;
    for (const AccessedStorageKey& key : item.storage_keys) {
      storage_keys.Append(base::Value(key));
    }
    access_list_item.Append(std::move(storage_keys));

    access_list.Append(std::move(access_list_item));
  }
  return access_list;
}

// static
std::optional<Eip2930Transaction::AccessList>
Eip2930Transaction::ValueToAccessList(const base::Value::List& value) {
  AccessList access_list;
  for (const auto& item_value : value) {
    AccessListItem item;
    std::vector<uint8_t> address = item_value.GetList()[0].GetBlob();
    std::move(address.begin(), address.end(), item.address.begin());
    for (const auto& storage_key_value : item_value.GetList()[1].GetList()) {
      std::vector<uint8_t> storage_key_vec = storage_key_value.GetBlob();
      AccessedStorageKey storage_key;
      std::move(storage_key_vec.begin(), storage_key_vec.end(),
                storage_key.begin());
      item.storage_keys.push_back(storage_key);
    }
    access_list.push_back(item);
  }
  return access_list;
}

std::vector<uint8_t> Eip2930Transaction::GetMessageToSign(
    uint256_t chain_id) const {
  DCHECK(nonce_);

  base::Value::List list;
  list.Append(RLPUint256ToBlob(chain_id_));
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(gas_price_));
  list.Append(RLPUint256ToBlob(gas_limit_));
  list.Append(base::Value::BlobStorage(to_.bytes()));
  list.Append(RLPUint256ToBlob(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));

  std::vector<uint8_t> result;
  result.push_back(type_);
  base::Extend(result, RLPEncode(list));
  return result;
}

std::string Eip2930Transaction::GetSignedTransaction() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(Serialize());
}

std::string Eip2930Transaction::GetTransactionHash() const {
  DCHECK(IsSigned());
  DCHECK(nonce_);

  return ToHex(KeccakHash(Serialize()));
}

void Eip2930Transaction::ProcessSignature(base::span<const uint8_t> signature,
                                          int recid,
                                          uint256_t chain_id) {
  EthTransaction::ProcessSignature(signature, recid, chain_id_);
  v_ = recid;
}

bool Eip2930Transaction::IsSigned() const {
  return r_.size() != 0 && s_.size() != 0;
}

base::Value::Dict Eip2930Transaction::ToValue() const {
  base::Value::Dict tx = EthTransaction::ToValue();
  tx.Set("chain_id", Uint256ValueToHex(chain_id_));
  tx.Set("access_list", base::Value(AccessListToValue(access_list_)));

  return tx;
}

uint256_t Eip2930Transaction::GetDataFee() const {
  uint256_t fee = EthTransaction::GetDataFee();

  for (const AccessListItem& item : access_list_) {
    fee += kAccessListAddressCost;
    fee += uint256_t(item.storage_keys.size()) * kAccessListStorageKeyCost;
  }
  return fee;
}

std::vector<uint8_t> Eip2930Transaction::Serialize() const {
  base::Value::List list;
  list.Append(RLPUint256ToBlob(chain_id_));
  list.Append(RLPUint256ToBlob(nonce_.value()));
  list.Append(RLPUint256ToBlob(gas_price_));
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

  base::Extend(result, RLPEncode(list));

  return result;
}

}  // namespace brave_wallet
