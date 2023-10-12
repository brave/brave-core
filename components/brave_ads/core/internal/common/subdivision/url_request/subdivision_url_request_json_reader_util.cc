/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_json_reader_util.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace brave_ads::json::reader {

namespace {

constexpr char kCountryKey[] = "country";
constexpr char kRegionKey[] = "region";

}  // namespace

absl::optional<std::string> ParseSubdivision(const std::string& json) {
  const absl::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(json);
  if (!dict) {
    return absl::nullopt;
  }

  const std::string* const country = dict->FindString(kCountryKey);
  if (!country || country->empty()) {
    return absl::nullopt;
  }

  const std::string* const region = dict->FindString(kRegionKey);
  if (!region || region->empty()) {
    return absl::nullopt;
  }

  return base::ReplaceStringPlaceholders("$1-$2", {*country, *region}, nullptr);
}

}  // namespace brave_ads::json::reader
