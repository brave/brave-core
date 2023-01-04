/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_JSON_READER_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_JSON_READER_UTIL_H_

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"

namespace ads::json::reader {

absl::optional<int> ParsePing(const base::Value& value);
absl::optional<IssuerList> ParseIssuers(const base::Value::Dict& value);

}  // namespace ads::json::reader

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_JSON_READER_UTIL_H_
