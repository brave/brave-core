/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader_util.h"

namespace ads::rewards::json::reader {

absl::optional<TransactionList> ReadTransactionHistory(
    const std::string& json) {
  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return absl::nullopt;
  }

  const absl::optional<TransactionList> transaction_history =
      ParseTransactionHistory(root->GetDict());
  if (!transaction_history) {
    return absl::nullopt;
  }

  return *transaction_history;
}

}  // namespace ads::rewards::json::reader
