/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_record_util.h"

#include <utility>

#include "base/check.h"

namespace brave_ads::database {

mojom::DBRecordInfoPtr CreateRecord(
    sql::Statement* statement,
    const std::vector<mojom::DBCommandInfo::RecordBindingType>& bindings) {
  CHECK(statement);

  mojom::DBRecordInfoPtr record = mojom::DBRecordInfo::New();

  int column = 0;

  for (const auto& binding : bindings) {
    CHECK(mojom::IsKnownEnumValue(binding));

    mojom::DBValuePtr value;
    switch (binding) {
      case mojom::DBCommandInfo::RecordBindingType::STRING_TYPE: {
        value = mojom::DBValue::NewStringValue(statement->ColumnString(column));
        break;
      }

      case mojom::DBCommandInfo::RecordBindingType::INT_TYPE: {
        value = mojom::DBValue::NewIntValue(statement->ColumnInt(column));
        break;
      }

      case mojom::DBCommandInfo::RecordBindingType::INT64_TYPE: {
        value = mojom::DBValue::NewInt64Value(statement->ColumnInt64(column));
        break;
      }

      case mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE: {
        value = mojom::DBValue::NewDoubleValue(statement->ColumnDouble(column));
        break;
      }

      case mojom::DBCommandInfo::RecordBindingType::BOOL_TYPE: {
        value = mojom::DBValue::NewBoolValue(statement->ColumnBool(column));
        break;
      }
    }

    record->fields.push_back(std::move(value));
    column++;
  }

  return record;
}

}  // namespace brave_ads::database
