/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database {

std::string BuildBindColumnPlaceholder(size_t column_count) {
  CHECK_NE(0U, column_count);

  const std::vector<std::string> bind_column_placeholders(column_count, "?");

  return base::ReplaceStringPlaceholders(
      "($1)", {base::JoinString(bind_column_placeholders, ", ")}, nullptr);
}

std::string BuildBindColumnPlaceholders(size_t column_count, size_t row_count) {
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
                const mojom::DBBindColumnInfo& mojom_db_bind_column) {
  CHECK(statement);

  switch (mojom_db_bind_column.value_union->which()) {
    case mojom::DBColumnValueUnion::Tag::kIntValue: {
      statement->BindInt(mojom_db_bind_column.index,
                         mojom_db_bind_column.value_union->get_int_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kInt64Value: {
      statement->BindInt64(mojom_db_bind_column.index,
                           mojom_db_bind_column.value_union->get_int64_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kDoubleValue: {
      statement->BindDouble(
          mojom_db_bind_column.index,
          mojom_db_bind_column.value_union->get_double_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kBoolValue: {
      statement->BindBool(mojom_db_bind_column.index,
                          mojom_db_bind_column.value_union->get_bool_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kStringValue: {
      statement->BindString(
          mojom_db_bind_column.index,
          mojom_db_bind_column.value_union->get_string_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kTimeValue: {
      statement->BindTime(mojom_db_bind_column.index,
                          mojom_db_bind_column.value_union->get_time_value());
      break;
    }

    case mojom::DBColumnValueUnion::Tag::kTimeDeltaValue: {
      statement->BindTimeDelta(
          mojom_db_bind_column.index,
          mojom_db_bind_column.value_union->get_time_delta_value());
      break;
    }
  }
}

void BindColumnInt(const mojom::DBActionInfoPtr& mojom_db_action,
                   int32_t index,
                   int32_t value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewIntValue(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

int ColumnInt(const mojom::DBRowInfoPtr& mojom_db_row, size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kIntValue,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_int_value();
}

void BindColumnInt64(const mojom::DBActionInfoPtr& mojom_db_action,
                     int32_t index,
                     int64_t value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewInt64Value(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

int64_t ColumnInt64(const mojom::DBRowInfoPtr& mojom_db_row, size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kInt64Value,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_int64_value();
}

void BindColumnDouble(const mojom::DBActionInfoPtr& mojom_db_action,
                      int32_t index,
                      double value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewDoubleValue(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

double ColumnDouble(const mojom::DBRowInfoPtr& mojom_db_row, size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kDoubleValue,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_double_value();
}

void BindColumnBool(const mojom::DBActionInfoPtr& mojom_db_action,
                    int32_t index,
                    bool value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewBoolValue(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

bool ColumnBool(const mojom::DBRowInfoPtr& mojom_db_row, size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kBoolValue,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_bool_value();
}

void BindColumnString(const mojom::DBActionInfoPtr& mojom_db_action,
                      int32_t index,
                      const std::string& value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewStringValue(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

std::string ColumnString(const mojom::DBRowInfoPtr& mojom_db_row,
                         size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kStringValue,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_string_value();
}

void BindColumnTime(const mojom::DBActionInfoPtr& mojom_db_action,
                    int32_t index,
                    base::Time value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewTimeValue(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

base::Time ColumnTime(const mojom::DBRowInfoPtr& mojom_db_row, size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kTimeValue,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_time_value();
}

void BindColumnTimeDelta(const mojom::DBActionInfoPtr& mojom_db_action,
                         int32_t index,
                         base::TimeDelta value) {
  CHECK(mojom_db_action);

  mojom::DBBindColumnInfoPtr mojom_db_bind_column =
      mojom::DBBindColumnInfo::New();
  mojom_db_bind_column->index = index;
  mojom_db_bind_column->value_union =
      mojom::DBColumnValueUnion::NewTimeDeltaValue(value);

  mojom_db_action->bind_columns.push_back(std::move(mojom_db_bind_column));
}

base::TimeDelta ColumnTimeDelta(const mojom::DBRowInfoPtr& mojom_db_row,
                                size_t column) {
  CHECK(mojom_db_row);
  CHECK_LT(column, mojom_db_row->column_values_union.size());
  CHECK_EQ(mojom::DBColumnValueUnion::Tag::kTimeDeltaValue,
           mojom_db_row->column_values_union.at(column)->which());

  return mojom_db_row->column_values_union.at(column)->get_time_delta_value();
}

}  // namespace brave_ads::database
