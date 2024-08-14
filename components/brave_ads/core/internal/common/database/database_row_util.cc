/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_row_util.h"

#include <utility>

#include "base/check.h"

namespace brave_ads::database {

mojom::DBRowInfoPtr CreateRow(
    sql::Statement* const statement,
    const std::vector<mojom::DBBindColumnType>& mojom_bind_column_types) {
  CHECK(statement);

  mojom::DBRowInfoPtr mojom_row = mojom::DBRowInfo::New();

  int column = 0;

  for (const auto& mojom_column_binding_type : mojom_bind_column_types) {
    mojom::DBColumnValueUnionPtr mojom_column_value_union;
    switch (mojom_column_binding_type) {
      case mojom::DBBindColumnType::kString: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewStringValue(
            statement->ColumnString(column));
        break;
      }

      case mojom::DBBindColumnType::kInt: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewIntValue(
            statement->ColumnInt(column));
        break;
      }

      case mojom::DBBindColumnType::kInt64: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewInt64Value(
            statement->ColumnInt64(column));
        break;
      }

      case mojom::DBBindColumnType::kDouble: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewDoubleValue(
            statement->ColumnDouble(column));
        break;
      }

      case mojom::DBBindColumnType::kBool: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewBoolValue(
            statement->ColumnBool(column));
        break;
      }

      case mojom::DBBindColumnType::kTime: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewTimeValue(
            statement->ColumnTime(column));
        break;
      }

      case mojom::DBBindColumnType::kTimeDelta: {
        mojom_column_value_union = mojom::DBColumnValueUnion::NewTimeDeltaValue(
            statement->ColumnTimeDelta(column));
        break;
      }
    }

    mojom_row->column_values_union.push_back(
        std::move(mojom_column_value_union));
    ++column;
  }

  return mojom_row;
}

}  // namespace brave_ads::database
