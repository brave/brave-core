/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database {

[[nodiscard]] int ColumnInt(mojom::DBRecordInfo* record, size_t index);
[[nodiscard]] int64_t ColumnInt64(mojom::DBRecordInfo* record, size_t index);
[[nodiscard]] double ColumnDouble(mojom::DBRecordInfo* record, size_t index);
[[nodiscard]] bool ColumnBool(mojom::DBRecordInfo* record, size_t index);
[[nodiscard]] std::string ColumnString(mojom::DBRecordInfo* record,
                                       size_t index);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
