/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/database/database_util.h"

namespace {

const int kCurrentVersionNumber = 32;
const int kCompatibleVersionNumber = 1;

}  // namespace

namespace ledger {
namespace database {

void BindNull(
    type::DBCommand* command,
    const int index) {
  if (!command) {
    return;
  }

  auto binding = type::DBCommandBinding::New();
  binding->index = index;
  binding->value = type::DBValue::New();
  binding->value->set_null_value(0);
  command->bindings.push_back(std::move(binding));
}

void BindInt(
    type::DBCommand* command,
    const int index,
    const int32_t value) {
  if (!command) {
    return;
  }

  auto binding = type::DBCommandBinding::New();
  binding->index = index;
  binding->value = type::DBValue::New();
  binding->value->set_int_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindInt64(
    type::DBCommand* command,
    const int index,
    const int64_t value) {
  if (!command) {
    return;
  }

  auto binding = type::DBCommandBinding::New();
  binding->index = index;
  binding->value = type::DBValue::New();
  binding->value->set_int64_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindDouble(
    type::DBCommand* command,
    const int index,
    const double value) {
  if (!command) {
    return;
  }

  auto binding = type::DBCommandBinding::New();
  binding->index = index;
  binding->value = type::DBValue::New();
  binding->value->set_double_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindBool(
    type::DBCommand* command,
    const int index,
    const bool value) {
  if (!command) {
    return;
  }

  auto binding = type::DBCommandBinding::New();
  binding->index = index;
  binding->value = type::DBValue::New();
  binding->value->set_bool_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindString(
    type::DBCommand* command,
    const int index,
    const std::string& value) {
  if (!command) {
    return;
  }

  auto binding = type::DBCommandBinding::New();
  binding->index = index;
  binding->value = type::DBValue::New();
  binding->value->set_string_value(value);
  command->bindings.push_back(std::move(binding));
}

int32_t GetCurrentVersion() {
  return kCurrentVersionNumber;
}

int32_t GetCompatibleVersion() {
  return kCompatibleVersionNumber;
}

void OnResultCallback(
    type::DBCommandResponsePtr response,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  callback(type::Result::LEDGER_OK);
}

int GetIntColumn(type::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (record->fields.at(index)->which() != type::DBValue::Tag::INT_VALUE) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int_value();
}

int64_t GetInt64Column(type::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (record->fields.at(index)->which() != type::DBValue::Tag::INT64_VALUE) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int64_value();
}

double GetDoubleColumn(type::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0.0;
  }

  if (record->fields.at(index)->which() != type::DBValue::Tag::DOUBLE_VALUE) {
    DCHECK(false);
    return 0.0;
  }

  return record->fields.at(index)->get_double_value();
}

bool GetBoolColumn(type::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return false;
  }

  if (record->fields.at(index)->which() != type::DBValue::Tag::BOOL_VALUE) {
    DCHECK(false);
    return false;
  }

  return record->fields.at(index)->get_bool_value();
}

std::string GetStringColumn(type::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return "";
  }

  if (record->fields.at(index)->which() != type::DBValue::Tag::STRING_VALUE) {
    DCHECK(false);
    return "";
  }

  return record->fields.at(index)->get_string_value();
}

std::string GenerateStringInCase(const std::vector<std::string>& items) {
  if (items.empty()) {
    return "";
  }

  const std::string items_join = base::JoinString(items, "\", \"");

  return base::StringPrintf("\"%s\"", items_join.c_str());
}

}  // namespace database
}  // namespace ledger
