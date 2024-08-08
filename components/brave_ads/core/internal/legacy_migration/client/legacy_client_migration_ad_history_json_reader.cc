/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_ad_history_json_reader.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_ad_history_json_reader_util.h"

namespace brave_ads::json::reader {

std::optional<AdHistoryList> ReadAdHistory(const std::string& json) {
  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(json);
  if (!dict) {
    return std::nullopt;
  }

  return ParseAdHistory(*dict);
}

}  // namespace brave_ads::json::reader
