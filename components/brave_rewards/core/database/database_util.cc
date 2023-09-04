/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_util.h"

namespace {

const int kCurrentVersionNumber = 40;
const int kCompatibleVersionNumber = 1;

}  // namespace

namespace brave_rewards::internal {
namespace database {

void BindNull(mojom::DBCommand* command, const int index) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewNullValue(0);
  command->bindings.push_back(std::move(binding));
}

void BindInt(mojom::DBCommand* command, const int index, const int32_t value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewIntValue(value);
  command->bindings.push_back(std::move(binding));
}

void BindInt64(mojom::DBCommand* command,
               const int index,
               const int64_t value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewInt64Value(value);
  command->bindings.push_back(std::move(binding));
}

void BindDouble(mojom::DBCommand* command,
                const int index,
                const double value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewDoubleValue(value);
  command->bindings.push_back(std::move(binding));
}

void BindBool(mojom::DBCommand* command, const int index, const bool value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewBoolValue(value);
  command->bindings.push_back(std::move(binding));
}

void BindString(mojom::DBCommand* command,
                const int index,
                const std::string& value) {
  if (!command) {
    return;
  }

  auto binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewStringValue(value);
  command->bindings.push_back(std::move(binding));
}

int32_t GetCurrentVersion() {
  return kCurrentVersionNumber;
}

int32_t GetCompatibleVersion() {
  return kCompatibleVersionNumber;
}

void OnResultCallback(LegacyResultCallback callback,
                      mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    callback(mojom::Result::FAILED);
    return;
  }

  callback(mojom::Result::OK);
}

int GetIntColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (!record->fields.at(index)->is_int_value()) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int_value();
}

int64_t GetInt64Column(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0;
  }

  if (!record->fields.at(index)->is_int64_value()) {
    DCHECK(false);
    return 0;
  }

  return record->fields.at(index)->get_int64_value();
}

double GetDoubleColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return 0.0;
  }

  if (!record->fields.at(index)->is_double_value()) {
    DCHECK(false);
    return 0.0;
  }

  return record->fields.at(index)->get_double_value();
}

bool GetBoolColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return false;
  }

  if (!record->fields.at(index)->is_bool_value()) {
    DCHECK(false);
    return false;
  }

  return record->fields.at(index)->get_bool_value();
}

std::string GetStringColumn(mojom::DBRecord* record, const int index) {
  if (!record || static_cast<int>(record->fields.size()) < index) {
    return "";
  }

  if (!record->fields.at(index)->is_string_value()) {
    DCHECK(false);
    return "";
  }

  return record->fields.at(index)->get_string_value();
}

std::string GenerateStringInCase(const std::vector<std::string>& items) {
  if (items.empty()) {
    return "";
  }

  const std::string items_join = base::JoinString(items, "', '");

  return base::StringPrintf("'%s'", items_join.c_str());
}

}  // namespace database
}  // namespace brave_rewards::internal
