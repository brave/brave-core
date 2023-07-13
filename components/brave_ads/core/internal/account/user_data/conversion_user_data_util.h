/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_UTIL_H_

#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct ConversionQueueItemInfo;

base::Value::Dict BuildConversionActionTypeUserData(
    const ConversionQueueItemInfo& conversion_queue_item);

absl::optional<base::Value::Dict> MaybeBuildVerifiableConversionUserData(
    const ConversionQueueItemInfo& conversion_queue_item);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_UTIL_H_
