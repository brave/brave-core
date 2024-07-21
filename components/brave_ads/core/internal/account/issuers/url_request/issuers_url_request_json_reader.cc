/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_json_reader.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_json_reader_util.h"

namespace brave_ads::json::reader {

std::optional<IssuersInfo> ReadIssuers(const std::string& json) {
  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(json);
  if (!dict) {
    return std::nullopt;
  }

  const std::optional<int> ping = ParsePing(*dict);
  if (!ping) {
    return std::nullopt;
  }

  const std::optional<TokenIssuerList> token_issuers = ParseTokenIssuers(*dict);
  if (!token_issuers) {
    return std::nullopt;
  }

  IssuersInfo issuers;
  issuers.ping = *ping;
  issuers.token_issuers = *token_issuers;

  return issuers;
}

}  // namespace brave_ads::json::reader
