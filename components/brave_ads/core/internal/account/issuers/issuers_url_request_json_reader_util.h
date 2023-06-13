/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_URL_REQUEST_JSON_READER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_URL_REQUEST_JSON_READER_UTIL_H_

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::json::reader {

absl::optional<int> ParsePing(const base::Value::Dict& dict);
absl::optional<IssuerList> ParseIssuers(const base::Value::Dict& dict);

}  // namespace brave_ads::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_URL_REQUEST_JSON_READER_UTIL_H_
