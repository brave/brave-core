/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_

#include <cstdint>
#include <string>

#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database {

int ColumnInt(mojom::DBRecordInfo* record, size_t index);
int64_t ColumnInt64(mojom::DBRecordInfo* record, size_t index);
double ColumnDouble(mojom::DBRecordInfo* record, size_t index);
bool ColumnBool(mojom::DBRecordInfo* record, size_t index);
std::string ColumnString(mojom::DBRecordInfo* record, size_t index);

}  // namespace ads::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
