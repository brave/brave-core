/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/database/database_bind_util.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database {

std::string BuildBindingParameterPlaceholder(const size_t parameters_count) {
  DCHECK_NE(0UL, parameters_count);

  const std::vector<std::string> placeholders(parameters_count, "?");

  return base::StringPrintf("(%s)",
                            base::JoinString(placeholders, ", ").c_str());
}

std::string BuildBindingParameterPlaceholders(const size_t parameters_count,
                                              const size_t values_count) {
  DCHECK_NE(0UL, values_count);

  std::string value = BuildBindingParameterPlaceholder(parameters_count);
  if (values_count == 1) {
    return value;
  }

  const std::vector<std::string> values(values_count, value);

  return base::JoinString(values, ", ");
}

void Bind(sql::Statement* statement,
          const mojom::DBCommandBindingInfo& binding) {
  DCHECK(statement);

  switch (binding.value->which()) {
    case mojom::DBValue::Tag::kNullValue: {
      statement->BindNull(binding.index);
      break;
    }

    case mojom::DBValue::Tag::kIntValue: {
      statement->BindInt(binding.index, binding.value->get_int_value());
      break;
    }

    case mojom::DBValue::Tag::kInt64Value: {
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      break;
    }

    case mojom::DBValue::Tag::kDoubleValue: {
      statement->BindDouble(binding.index, binding.value->get_double_value());
      break;
    }

    case mojom::DBValue::Tag::kBoolValue: {
      statement->BindBool(binding.index, binding.value->get_bool_value());
      break;
    }

    case mojom::DBValue::Tag::kStringValue: {
      statement->BindString(binding.index, binding.value->get_string_value());
      break;
    }
  }
}

void BindNull(mojom::DBCommandInfo* command, const int_fast16_t index) {
  DCHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewNullValue(0);

  command->bindings.push_back(std::move(binding));
}

void BindInt(mojom::DBCommandInfo* command,
             const int index,
             const int32_t value) {
  DCHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewIntValue(value);

  command->bindings.push_back(std::move(binding));
}

void BindInt64(mojom::DBCommandInfo* command,
               const int index,
               const int64_t value) {
  DCHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewInt64Value(value);

  command->bindings.push_back(std::move(binding));
}

void BindDouble(mojom::DBCommandInfo* command,
                const int index,
                const double value) {
  DCHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewDoubleValue(value);

  command->bindings.push_back(std::move(binding));
}

void BindBool(mojom::DBCommandInfo* command,
              const int index,
              const bool value) {
  DCHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewBoolValue(value);

  command->bindings.push_back(std::move(binding));
}

void BindString(mojom::DBCommandInfo* command,
                const int index,
                const std::string& value) {
  DCHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewStringValue(value);

  command->bindings.push_back(std::move(binding));
}

}  // namespace ads::database
