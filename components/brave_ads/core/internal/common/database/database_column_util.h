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
#include "sql/statement.h"

namespace brave_ads::database {

std::string BuildBindColumnPlaceholder(size_t column_count);
std::string BuildBindColumnPlaceholders(size_t column_count, size_t row_count);

void BindColumn(sql::Statement* statement,
                const mojom::DBBindColumnInfo& mojom_bind_column);

void BindColumnInt(mojom::DBStatementInfo* mojom_statement,
                   int32_t index,
                   int32_t value);
[[nodiscard]] int ColumnInt(const mojom::DBRowInfo* mojom_row, size_t column);

void BindColumnInt64(mojom::DBStatementInfo* mojom_statement,
                     int32_t index,
                     int64_t value);
[[nodiscard]] int64_t ColumnInt64(const mojom::DBRowInfo* mojom_row,
                                  size_t column);

void BindColumnDouble(mojom::DBStatementInfo* mojom_statement,
                      int32_t index,
                      double value);
[[nodiscard]] double ColumnDouble(const mojom::DBRowInfo* mojom_row,
                                  size_t column);

void BindColumnBool(mojom::DBStatementInfo* mojom_statement,
                    int32_t index,
                    bool value);
[[nodiscard]] bool ColumnBool(const mojom::DBRowInfo* mojom_row, size_t column);

void BindColumnString(mojom::DBStatementInfo* mojom_statement,
                      int32_t index,
                      const std::string& value);
[[nodiscard]] std::string ColumnString(const mojom::DBRowInfo* mojom_row,
                                       size_t column);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_COLUMN_UTIL_H_
