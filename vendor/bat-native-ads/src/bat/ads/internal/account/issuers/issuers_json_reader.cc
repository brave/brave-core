/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_json_reader.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_json_reader_util.h"

namespace ads::json::reader {

absl::optional<IssuersInfo> ReadIssuers(const std::string& json) {
  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return absl::nullopt;
  }

  const absl::optional<int> ping = ParsePing(*root);
  if (!ping) {
    return absl::nullopt;
  }

  const absl::optional<IssuerList> issuers = ParseIssuers(root->GetDict());
  if (!issuers) {
    return absl::nullopt;
  }

  IssuersInfo new_issuers;
  new_issuers.ping = *ping;
  new_issuers.issuers = *issuers;

  return new_issuers;
}

}  // namespace ads::json::reader
