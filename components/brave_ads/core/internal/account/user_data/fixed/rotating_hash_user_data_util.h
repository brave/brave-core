/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_ROTATING_HASH_USER_DATA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_ROTATING_HASH_USER_DATA_UTIL_H_

#include <optional>
#include <string>

namespace brave_ads {

struct TransactionInfo;

std::optional<std::string> BuildRotatingHash(
    const TransactionInfo& transaction);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_ROTATING_HASH_USER_DATA_UTIL_H_
