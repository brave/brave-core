/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eip2930_transaction.h"

#include <utility>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/rlp_encode.h"

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
  if (!std::equal(address.begin(), address.end(), item.address.begin()))
    return false;
  if (storage_keys.size() != item.storage_keys.size())
    return false;
  for (size_t i = 0; i < storage_keys.size(); ++i) {
    if (!std::equal(storage_keys[i].begin(), storage_keys[i].end(),
                    item.storage_keys[i].begin()))
      return false;
  }
  return true;
}

bool Eip2930Transaction::AccessListItem::operator!=(
    const AccessListItem& item) const {
  return !operator==(item);
}

Eip2930Transaction::Eip2930Transaction() = default;
Eip2930Transaction::Eip2930Transaction(mojom::TxDataPtr tx_data,
                                       const std::string& chain_id)
    : EthTransaction(std::move(tx_data)), chain_id_(chain_id) {
  type_ = 1;
}

Eip2930Transaction::~Eip2930Transaction() = default;

bool Eip2930Transaction::operator==(const Eip2930Transaction& tx) const {
  return EthTransaction::operator==(tx) && chain_id_ == tx.chain_id_ &&
         std::equal(access_list_.begin(), access_list_.end(),
                    tx.access_list_.begin());
}

// static
std::unique_ptr<Eip2930Transaction> Eip2930Transaction::FromValue(
    const base::Value& value) {
  auto legacy_tx = EthTransaction::FromValue(value);
  if (!legacy_tx)
    return nullptr;
  auto tx_data = mojom::TxData::New(legacy_tx->nonce(), legacy_tx->gas_price(),
                                    legacy_tx->gas_limit(), legacy_tx->to(),
                                    legacy_tx->value(), legacy_tx->data());

  const std::string* tx_chain_id = value.FindStringKey("chain_id");
  if (!tx_chain_id)
    return nullptr;

  auto tx =
      std::make_unique<Eip2930Transaction>(std::move(tx_data), *tx_chain_id);
  tx->v_ = legacy_tx->v();
  tx->r_ = legacy_tx->r();
  tx->s_ = legacy_tx->s();

  const base::Value* access_list = value.FindKey("access_list");
  if (!access_list)
    return nullptr;
  absl::optional<AccessList> access_list_from_value =
      ValueToAccessList(*access_list);
  if (!access_list_from_value)
    return nullptr;
  tx->access_list_ = *access_list_from_value;

  return tx;
}

// static
std::vector<base::Value> Eip2930Transaction::AccessListToValue(
    const AccessList& list) {
  std::vector<base::Value> access_list;
  for (const AccessListItem& item : list) {
    std::vector<base::Value> access_list_item;
    access_list_item.push_back(base::Value(item.address));
    std::vector<base::Value> storage_keys;
    for (const AccessedStorageKey& key : item.storage_keys) {
      storage_keys.push_back(base::Value(key));
    }
    access_list_item.push_back(base::Value(storage_keys));

    access_list.push_back(base::Value(access_list_item));
  }
  return access_list;
}

// static
absl::optional<Eip2930Transaction::AccessList>
Eip2930Transaction::ValueToAccessList(const base::Value& value) {
  AccessList access_list;
  for (const auto& item_value : value.GetList()) {
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

bool Eip2930Transaction::GetBasicListData(base::ListValue* list) const {
  CHECK(list);
  uint256_t chain_id_uint;
  if (!HexValueToUint256(chain_id_, &chain_id_uint)) {
    return false;
  }
  list->Append(RLPUint256ToBlobValue(chain_id_uint));
  if (!EthTransaction::GetBasicListData(list)) {
    return false;
  }
  list->Append(base::Value(AccessListToValue(access_list_)));
  return true;
}

void Eip2930Transaction::GetMessageToSign(const std::string& chain_id,
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

void Eip2930Transaction::GetSignedTransaction(
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

void Eip2930Transaction::ProcessSignature(const std::vector<uint8_t>& signature,
                                          int recid,
                                          const std::string& chain_id) {
  EthTransaction::ProcessSignature(signature, recid, chain_id_);
  v_ = recid;
}

bool Eip2930Transaction::IsSigned() const {
  return r_.size() != 0 && s_.size() != 0;
}

base::Value Eip2930Transaction::ToValue() const {
  base::Value tx = EthTransaction::ToValue();
  tx.SetStringKey("chain_id", chain_id_);
  tx.SetKey("access_list", base::Value(AccessListToValue(access_list_)));

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

}  // namespace brave_wallet
