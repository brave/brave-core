/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CREATED_AT_TIMESTAMP_USER_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CREATED_AT_TIMESTAMP_USER_DATA_H_

#include "base/values.h"

namespace base {
class Time;
}  // namespace base

namespace ads::user_data {

base::Value::Dict GetCreatedAtTimestamp(base::Time created_at);

}  // namespace ads::user_data

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CREATED_AT_TIMESTAMP_USER_DATA_H_
