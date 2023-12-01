/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_json_reader_util.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_value_util.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kPingKey[] = "ping";
constexpr char kIssuersKey[] = "issuers";

}  // namespace

std::optional<int> ParsePing(const base::Value::Dict& dict) {
  return dict.FindInt(kPingKey);
}

std::optional<IssuerList> ParseIssuers(const base::Value::Dict& dict) {
  const auto* const list = dict.FindList(kIssuersKey);
  if (!list) {
    return std::nullopt;
  }

  return ValueToIssuers(*list);
}

}  // namespace brave_ads::json::reader
