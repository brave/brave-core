/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_TOTALS_USER_DATA_ALIAS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_TOTALS_USER_DATA_ALIAS_H_

#include <string>

#include "base/containers/flat_map.h"

namespace ads::user_data {

using ConfirmationTypeBucketMap = base::flat_map<std::string, int>;
using AdTypeBucketMap = base::flat_map<std::string, ConfirmationTypeBucketMap>;

}  // namespace ads::user_data

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_TOTALS_USER_DATA_ALIAS_H_
