/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_VALUE_UTIL_H_

#include <optional>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"

namespace brave_ads {

base::Value::List IssuersToValue(const IssuerList& issuers);
std::optional<IssuerList> ValueToIssuers(const base::Value::List& list);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_VALUE_UTIL_H_
