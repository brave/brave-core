/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/database/database_util.h"

namespace {

const int kCurrentVersionNumber = 29;
const int kCompatibleVersionNumber = 1;

}  // namespace

namespace braveledger_database {

void BindNull(
    ledger::DBCommand* command,
    const int index) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_null_value(0);
  command->bindings.push_back(std::move(binding));
}

void BindInt(
    ledger::DBCommand* command,
    const int index,
    const int32_t value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_int_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindInt64(
    ledger::DBCommand* command,
    const int index,
    const int64_t value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_int64_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindDouble(
    ledger::DBCommand* command,
    const int index,
    const double value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_double_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindBool(
    ledger::DBCommand* command,
    const int index,
    const bool value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
  binding->value->set_bool_value(value);
  command->bindings.push_back(std::move(binding));
}

void BindString(
    ledger::DBCommand* command,
    const int index,
    const std::string& value) {
  if (!command) {
    return;
  }

  auto binding = ledger::DBCommandBinding::New();
  binding->index = index;
  binding->value = ledger::DBValue::New();
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
    ledger::DBCommandResponsePtr response,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

int GetIntColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::INT_VALUE) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int_value();
}

int64_t GetInt64Column(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::INT64_VALUE) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int64_value();
}

double GetDoubleColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0.0;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::DOUBLE_VALUE) {
    DCHECK(false);
    return 0.0;
  }

  return record->fields.at(index)->get_double_value();
}

bool GetBoolColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return false;
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::BOOL_VALUE) {
    DCHECK(false);
    return false;
  }

  return record->fields.at(index)->get_bool_value();
}

std::string GetStringColumn(ledger::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return "";
  }

  if (record->fields.at(index)->which() != ledger::DBValue::Tag::STRING_VALUE) {
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

}  // namespace braveledger_database
