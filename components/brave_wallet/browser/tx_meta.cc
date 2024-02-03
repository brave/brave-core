/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_meta.h"

#include "base/json/values_util.h"
#include "base/uuid.h"
#include "base/values.h"

namespace brave_wallet {

TxMeta::TxMeta() = default;
TxMeta::~TxMeta() = default;

bool TxMeta::operator==(const TxMeta& meta) const {
  if (GetCoinType() != meta.GetCoinType()) {
    return false;
  }

  return id_ == meta.id_ && status_ == meta.status_ && from_ == meta.from_ &&
         created_time_ == meta.created_time_ &&
         submitted_time_ == meta.submitted_time_ &&
         confirmed_time_ == meta.confirmed_time_ && tx_hash_ == meta.tx_hash_ &&
         origin_ == meta.origin_ && chain_id_ == meta.chain_id_;
}

base::Value::Dict TxMeta::ToValue() const {
  base::Value::Dict dict;

  dict.Set("id", id_);
  dict.Set("status", static_cast<int>(status_));
  dict.Set("from_account_id", from_->unique_key);
  dict.Set("created_time", base::TimeToValue(created_time_));
  dict.Set("submitted_time", base::TimeToValue(submitted_time_));
  dict.Set("confirmed_time", base::TimeToValue(confirmed_time_));
  dict.Set("tx_hash", tx_hash_);
  if (origin_.has_value()) {
    DCHECK(!origin_->opaque());
    dict.Set("origin", origin_->GetURL().spec());
  }
  dict.Set("coin", static_cast<int>(GetCoinType()));
  dict.Set("chain_id", chain_id_);
  return dict;
}

// static
std::string TxMeta::GenerateMetaID() {
  return base::Uuid::GenerateRandomV4().AsLowercaseString();
}

}  // namespace brave_wallet
