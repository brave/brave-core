/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_COLUMN_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_COLUMN_UTIL_H_

#include <cstdint>
#include <string>

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {

int ColumnInt(mojom::DBRecordInfo* record, const size_t index);
int64_t ColumnInt64(mojom::DBRecordInfo* record, const size_t index);
double ColumnDouble(mojom::DBRecordInfo* record, const size_t index);
bool ColumnBool(mojom::DBRecordInfo* record, const size_t index);
std::string ColumnString(mojom::DBRecordInfo* record, const size_t index);

}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_COLUMN_UTIL_H_
