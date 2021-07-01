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

Eip2930Transaction::AccessListItem::AccessListItem() = default;
Eip2930Transaction::AccessListItem::~AccessListItem() = default;
Eip2930Transaction::AccessListItem::AccessListItem(const AccessListItem&) =
    default;

Eip2930Transaction::Eip2930Transaction() = default;
Eip2930Transaction::Eip2930Transaction(const TxData& tx_data, uint64_t chain_id)
    : EthTransaction(tx_data), chain_id_(chain_id) {
  type_ = 1;
}
Eip2930Transaction::Eip2930Transaction(const Eip2930Transaction&) = default;
Eip2930Transaction::~Eip2930Transaction() = default;

bool Eip2930Transaction::operator==(const Eip2930Transaction& tx) const {
  return EthTransaction::operator==(tx) && chain_id_ == tx.chain_id_;
}

std::vector<base::Value> Eip2930Transaction::AccessListToValue(
    const AccessList& list) const {
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

std::vector<uint8_t> Eip2930Transaction::GetMessageToSign(
    uint64_t chain_id) const {
  std::vector<uint8_t> result;
  result.push_back(type_);

  // TODO(darkdh): Migrate to std::vector<base::Value>, base::ListValue is
  // deprecated
  base::ListValue list;
  list.Append(RLPUint256ToBlobValue(chain_id_));
  list.Append(RLPUint256ToBlobValue(nonce_));
  list.Append(RLPUint256ToBlobValue(gas_price_));
  list.Append(RLPUint256ToBlobValue(gas_limit_));
  list.Append(base::Value(to_.bytes()));
  list.Append(RLPUint256ToBlobValue(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));

  const std::string rlp_msg = RLPEncode(std::move(list));
  result.insert(result.end(), rlp_msg.begin(), rlp_msg.end());
  return KeccakHash(result);
}

std::string Eip2930Transaction::GetSignedTransaction() const {
  DCHECK(IsSigned());

  // TODO(darkdh): Migrate to std::vector<base::Value>, base::ListValue is
  // deprecated
  base::ListValue list;
  list.Append(RLPUint256ToBlobValue(chain_id_));
  list.Append(RLPUint256ToBlobValue(nonce_));
  list.Append(RLPUint256ToBlobValue(gas_price_));
  list.Append(RLPUint256ToBlobValue(gas_limit_));
  list.Append(base::Value(to_.bytes()));
  list.Append(RLPUint256ToBlobValue(value_));
  list.Append(base::Value(data_));
  list.Append(base::Value(AccessListToValue(access_list_)));
  list.Append(base::Value(v_));
  list.Append(base::Value(r_));
  list.Append(base::Value(s_));

  std::vector<uint8_t> result;
  result.push_back(type_);

  const std::string rlp_msg = RLPEncode(std::move(list));
  result.insert(result.end(), rlp_msg.begin(), rlp_msg.end());

  return ToHex(result);
}

void Eip2930Transaction::ProcessSignature(const std::vector<uint8_t> signature,
                                          int recid,
                                          uint64_t chain_id) {
  EthTransaction::ProcessSignature(signature, recid, chain_id_);
  v_ = recid;
}

bool Eip2930Transaction::IsSigned() const {
  return r_.size() != 0 && s_.size() != 0;
}

}  // namespace brave_wallet
