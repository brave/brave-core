/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "sql/statement.h"

namespace brave_ads::database {

std::string BuildBindColumnPlaceholder(size_t column_count);
std::string BuildBindColumnPlaceholders(size_t column_count, size_t row_count);

void BindColumn(sql::Statement* statement,
                const mojom::DBBindColumnInfo& mojom_db_bind_column);

void BindColumnInt(const mojom::DBActionInfoPtr& mojom_db_action,
                   int32_t index,
                   int32_t value);
[[nodiscard]] int ColumnInt(const mojom::DBRowInfoPtr& mojom_db_row,
                            size_t column);

void BindColumnInt64(const mojom::DBActionInfoPtr& mojom_db_action,
                     int32_t index,
                     int64_t value);
[[nodiscard]] int64_t ColumnInt64(const mojom::DBRowInfoPtr& mojom_db_row,
                                  size_t column);

void BindColumnDouble(const mojom::DBActionInfoPtr& mojom_db_action,
                      int32_t index,
                      double value);
[[nodiscard]] double ColumnDouble(const mojom::DBRowInfoPtr& mojom_db_row,
                                  size_t column);

void BindColumnBool(const mojom::DBActionInfoPtr& mojom_db_action,
                    int32_t index,
                    bool value);
[[nodiscard]] bool ColumnBool(const mojom::DBRowInfoPtr& mojom_db_row,
                              size_t column);

void BindColumnString(const mojom::DBActionInfoPtr& mojom_db_action,
                      int32_t index,
                      std::string_view value);
[[nodiscard]] std::string ColumnString(const mojom::DBRowInfoPtr& mojom_db_row,
                                       size_t column);

void BindColumnTime(const mojom::DBActionInfoPtr& mojom_db_action,
                    int32_t index,
                    base::Time value);
[[nodiscard]] base::Time ColumnTime(const mojom::DBRowInfoPtr& mojom_db_row,
                                    size_t column);

void BindColumnTimeDelta(const mojom::DBActionInfoPtr& mojom_db_action,
                         int32_t index,
                         base::TimeDelta value);
[[nodiscard]] base::TimeDelta ColumnTimeDelta(
    const mojom::DBRowInfoPtr& mojom_db_row,
    size_t column);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
