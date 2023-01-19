/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_DIAGNOSTIC_ID_USER_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_DIAGNOSTIC_ID_USER_DATA_H_

#include "base/values.h"

namespace ads::user_data {

base::Value::Dict GetDiagnosticId();

}  // namespace ads::user_data

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_DIAGNOSTIC_ID_USER_DATA_H_
