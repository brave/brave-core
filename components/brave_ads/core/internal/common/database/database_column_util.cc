/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database {

std::string BuildBindColumnPlaceholder(const size_t column_count) {
  CHECK_NE(0U, column_count);

  const std::vector<std::string> bind_column_placeholders(column_count, "?");

  return base::ReplaceStringPlaceholders(
      "($1)", {base::JoinString(bind_column_placeholders, ", ")}, nullptr);
}

std::string BuildBindColumnPlaceholders(const size_t column_count,
                                        const size_t row_count) {
  CHECK_NE(0U, column_count);
  CHECK_NE(0U, row_count);

  std::string bind_column_placeholder =
      BuildBindColumnPlaceholder(column_count);
  if (row_count == 1) {
    return bind_column_placeholder;
  }

  const std::vector<std::string> bind_column_placeholders(
      row_count, bind_column_placeholder);

  return base::JoinString(bind_column_placeholders, ", ");
}

void BindColumn(sql::Statement* const statement,
                const mojom::DBBindColumnInfo& mojom_bind_column) {
  CHECK(statement);

  switch (mojom_bind_column.value_union->which()) {
    case mojom::DBColumnValueUnion::Tag::kIntValue: {
      statement->BindInt(mojom_bind_column.index,
                         mojom_bind_column.value_union->get_int_value());
      break;
    }
    case mojom::DBColumnValueUnion::Tag::kInt64Value: {
      statement->BindInt64(mojom_bind_column.index,
                           mojom_bind_column.value_union->get_int64_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kDoubleValue: {
      statement->BindDouble(mojom_bind_column.index,
                            mojom_bind_column.value_union->get_double_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kBoolValue: {
      statement->BindBool(mojom_bind_column.index,
                          mojom_bind_column.value_union->get_bool_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kStringValue: {
      statement->BindString(mojom_bind_column.index,
                            mojom_bind_column.value_union->get_string_value());
      break;
    }
  }
}

void BindColumnInt(mojom::DBStatementInfo* const mojom_statement,
                   const int32_t index,
                   const int32_t value) {
  CHECK(mojom_statement);

  mojom::DBBindColumnInfoPtr mojom_bind_column = mojom::DBBindColumnInfo::New();
  mojom_bind_column->index = index;
  mojom_bind_column->value_union =
      mojom::DBColumnValueUnion::NewIntValue(value);

  mojom_statement->bind_columns.push_back(std::move(mojom_bind_column));
}

int ColumnInt(const mojom::DBRowInfo* const mojom_row, const size_t column) {
  CHECK(mojom_row);
  CHECK_LT(column, mojom_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kIntValue,
           mojom_row->column_values_union.at(column)->which());

  return mojom_row->column_values_union.at(column)->get_int_value();
}

void BindColumnInt64(mojom::DBStatementInfo* const mojom_statement,
                     const int32_t index,
                     const int64_t value) {
  CHECK(mojom_statement);

  mojom::DBBindColumnInfoPtr mojom_bind_column = mojom::DBBindColumnInfo::New();
  mojom_bind_column->index = index;
  mojom_bind_column->value_union =
      mojom::DBColumnValueUnion::NewInt64Value(value);

  mojom_statement->bind_columns.push_back(std::move(mojom_bind_column));
}

int64_t ColumnInt64(const mojom::DBRowInfo* const mojom_row,
                    const size_t column) {
  CHECK(mojom_row);
  CHECK_LT(column, mojom_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kInt64Value,
           mojom_row->column_values_union.at(column)->which());

  return mojom_row->column_values_union.at(column)->get_int64_value();
}

void BindColumnDouble(mojom::DBStatementInfo* const mojom_statement,
                      const int32_t index,
                      const double value) {
  CHECK(mojom_statement);

  mojom::DBBindColumnInfoPtr mojom_bind_column = mojom::DBBindColumnInfo::New();
  mojom_bind_column->index = index;
  mojom_bind_column->value_union =
      mojom::DBColumnValueUnion::NewDoubleValue(value);

  mojom_statement->bind_columns.push_back(std::move(mojom_bind_column));
}

double ColumnDouble(const mojom::DBRowInfo* const mojom_row,
                    const size_t column) {
  CHECK(mojom_row);
  CHECK_LT(column, mojom_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kDoubleValue,
           mojom_row->column_values_union.at(column)->which());

  return mojom_row->column_values_union.at(column)->get_double_value();
}

void BindColumnBool(mojom::DBStatementInfo* const mojom_statement,
                    const int32_t index,
                    const bool value) {
  CHECK(mojom_statement);

  mojom::DBBindColumnInfoPtr mojom_bind_column = mojom::DBBindColumnInfo::New();
  mojom_bind_column->index = index;
  mojom_bind_column->value_union =
      mojom::DBColumnValueUnion::NewBoolValue(value);

  mojom_statement->bind_columns.push_back(std::move(mojom_bind_column));
}

bool ColumnBool(const mojom::DBRowInfo* const mojom_row, const size_t column) {
  CHECK(mojom_row);
  CHECK_LT(column, mojom_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kBoolValue,
           mojom_row->column_values_union.at(column)->which());

  return mojom_row->column_values_union.at(column)->get_bool_value();
}

void BindColumnString(mojom::DBStatementInfo* const mojom_statement,
                      const int32_t index,
                      const std::string& value) {
  CHECK(mojom_statement);

  mojom::DBBindColumnInfoPtr mojom_bind_column = mojom::DBBindColumnInfo::New();
  mojom_bind_column->index = index;
  mojom_bind_column->value_union =
      mojom::DBColumnValueUnion::NewStringValue(value);

  mojom_statement->bind_columns.push_back(std::move(mojom_bind_column));
}

std::string ColumnString(const mojom::DBRowInfo* const mojom_row,
                         const size_t column) {
  CHECK(mojom_row);
  CHECK_LT(column, mojom_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kStringValue,
           mojom_row->column_values_union.at(column)->which());

  return mojom_row->column_values_union.at(column)->get_string_value();
}

}  // namespace brave_ads::database
