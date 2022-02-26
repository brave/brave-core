/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_meta.h"

#include "base/guid.h"
#include "base/json/values_util.h"
#include "base/values.h"

namespace brave_wallet {

TxMeta::TxMeta() = default;

bool TxMeta::operator==(const TxMeta& meta) const {
  return id_ == meta.id_ && status_ == meta.status_ && from_ == meta.from_ &&
         created_time_ == meta.created_time_ &&
         submitted_time_ == meta.submitted_time_ &&
         confirmed_time_ == meta.confirmed_time_ && tx_hash_ == meta.tx_hash_;
}

base::Value TxMeta::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);

  dict.SetStringKey("id", id_);
  dict.SetIntKey("status", static_cast<int>(status_));
  dict.SetStringKey("from", from_);
  dict.SetKey("created_time", base::TimeToValue(created_time_));
  dict.SetKey("submitted_time", base::TimeToValue(submitted_time_));
  dict.SetKey("confirmed_time", base::TimeToValue(confirmed_time_));
  dict.SetStringKey("tx_hash", tx_hash_);

  return dict;
}

// static
std::string TxMeta::GenerateMetaID() {
  return base::GenerateGUID();
}

}  // namespace brave_wallet
