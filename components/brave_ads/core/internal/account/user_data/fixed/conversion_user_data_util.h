/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_CONVERSION_USER_DATA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_CONVERSION_USER_DATA_UTIL_H_

#include <optional>

#include "base/values.h"

namespace brave_ads {

struct ConversionInfo;

base::DictValue BuildConversionActionTypeUserData(
    const ConversionInfo& conversion);

// Returns `std::nullopt` if neither `conversion.order_id` nor
// `conversion.order_value` is set, so legacy conversions are unaffected.
std::optional<base::DictValue> MaybeBuildConversionOrderUserData(
    const ConversionInfo& conversion);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_FIXED_CONVERSION_USER_DATA_UTIL_H_
