/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"

namespace brave_ads::database {

std::string BuildBindingParameterPlaceholder(const size_t parameters_count) {
  CHECK_NE(0U, parameters_count);

  const std::vector<std::string> placeholders(parameters_count, "?");

  return base::ReplaceStringPlaceholders(
      "($1)", {base::JoinString(placeholders, ", ")}, nullptr);
}

std::string BuildBindingParameterPlaceholders(
    const size_t parameters_count,
    const size_t binded_parameters_count) {
  CHECK_NE(0U, binded_parameters_count);

  std::string placeholder = BuildBindingParameterPlaceholder(parameters_count);
  if (binded_parameters_count == 1) {
    return placeholder;
  }

  const std::vector<std::string> placeholders(binded_parameters_count,
                                              placeholder);

  return base::JoinString(placeholders, ", ");
}

void Bind(sql::Statement* statement,
          const mojom::DBCommandBindingInfo& binding) {
  CHECK(statement);

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

void BindNull(mojom::DBCommandInfo* command, const int32_t index) {
  CHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewNullValue(0);

  command->bindings.push_back(std::move(binding));
}

void BindInt(mojom::DBCommandInfo* command,
             const int32_t index,
             const int32_t value) {
  CHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewIntValue(value);

  command->bindings.push_back(std::move(binding));
}

void BindInt64(mojom::DBCommandInfo* command,
               const int32_t index,
               const int64_t value) {
  CHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewInt64Value(value);

  command->bindings.push_back(std::move(binding));
}

void BindDouble(mojom::DBCommandInfo* command,
                const int32_t index,
                const double value) {
  CHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewDoubleValue(value);

  command->bindings.push_back(std::move(binding));
}

void BindBool(mojom::DBCommandInfo* command,
              const int32_t index,
              const bool value) {
  CHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewBoolValue(value);

  command->bindings.push_back(std::move(binding));
}

void BindString(mojom::DBCommandInfo* command,
                const int32_t index,
                const std::string& value) {
  CHECK(command);

  mojom::DBCommandBindingInfoPtr binding = mojom::DBCommandBindingInfo::New();
  binding->index = index;
  binding->value = mojom::DBValue::NewStringValue(value);

  command->bindings.push_back(std::move(binding));
}

}  // namespace brave_ads::database
