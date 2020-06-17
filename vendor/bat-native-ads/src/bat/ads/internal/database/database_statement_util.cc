/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_statement_util.h"

#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {

std::string BuildBindingParameterPlaceholder(
    const size_t parameters_count) {
  DCHECK_NE(0UL, parameters_count);

  const std::vector<std::string> placeholders(parameters_count, "?");

  return base::StringPrintf("(%s)",
      base::JoinString(placeholders, ", ").c_str());
}

std::string BuildBindingParameterPlaceholders(
    const size_t parameters_count,
    const size_t values_count) {
  DCHECK_NE(0UL, values_count);

  const std::string value = BuildBindingParameterPlaceholder(parameters_count);
  if (values_count == 1) {
    return value;
  }

  const std::vector<std::string> values(values_count, value);

  return base::JoinString(values, ", ");
}

void BindNull(
    DBCommand* command,
    const int_fast16_t index) {
  DCHECK(command);

  DBCommandBindingPtr binding = DBCommandBinding::New();
  binding->index = index;
  binding->value = DBValue::New();
  binding->value->set_null_value(0);

  command->bindings.push_back(std::move(binding));
}

void BindInt(
    DBCommand* command,
    const int index,
    const int32_t value) {
  DCHECK(command);

  DBCommandBindingPtr binding = DBCommandBinding::New();
  binding->index = index;
  binding->value = DBValue::New();
  binding->value->set_int_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindInt64(
    DBCommand* command,
    const int index,
    const int64_t value) {
  DCHECK(command);

  DBCommandBindingPtr binding = DBCommandBinding::New();
  binding->index = index;
  binding->value = DBValue::New();
  binding->value->set_int64_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindDouble(
    DBCommand* command,
    const int index,
    const double value) {
  DCHECK(command);

  DBCommandBindingPtr binding = DBCommandBinding::New();
  binding->index = index;
  binding->value = DBValue::New();
  binding->value->set_double_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindBool(
    DBCommand* command,
    const int index,
    const bool value) {
  DCHECK(command);

  DBCommandBindingPtr binding = DBCommandBinding::New();
  binding->index = index;
  binding->value = DBValue::New();
  binding->value->set_bool_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindString(
    DBCommand* command,
    const int index,
    const std::string& value) {
  DCHECK(command);

  DBCommandBindingPtr binding = DBCommandBinding::New();
  binding->index = index;
  binding->value = DBValue::New();
  binding->value->set_string_value(value);

  command->bindings.push_back(std::move(binding));
}

int ColumnInt(
    DBRecord* record,
    const size_t index) {
  DCHECK(record);
  DCHECK_LT(index, record->fields.size());
  DCHECK_EQ(DBValue::Tag::INT_VALUE, record->fields.at(index)->which());

  return record->fields.at(index)->get_int_value();
}

int64_t ColumnInt64(
    DBRecord* record,
    const size_t index) {
  DCHECK(record);
  DCHECK_LT(index, record->fields.size());
  DCHECK_EQ(DBValue::Tag::INT64_VALUE, record->fields.at(index)->which());

  return record->fields.at(index)->get_int64_value();
}

double ColumnDouble(
    DBRecord* record,
    const size_t index) {
  DCHECK(record);
  DCHECK_LT(index, record->fields.size());
  DCHECK_EQ(DBValue::Tag::DOUBLE_VALUE, record->fields.at(index)->which());

  return record->fields.at(index)->get_double_value();
}

bool ColumnBool(
    DBRecord* record,
    const size_t index) {
  DCHECK(record);
  DCHECK_LT(index, record->fields.size());
  DCHECK_EQ(DBValue::Tag::BOOL_VALUE, record->fields.at(index)->which());

  return record->fields.at(index)->get_bool_value();
}

std::string ColumnString(
    DBRecord* record,
    const size_t index) {
  DCHECK(record);
  DCHECK_LT(index, record->fields.size());
  DCHECK_EQ(DBValue::Tag::STRING_VALUE, record->fields.at(index)->which());

  return record->fields.at(index)->get_string_value();
}

}  // namespace database
}  // namespace ads
