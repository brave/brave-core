/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/database_bind_util.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace ads {
namespace database {

std::string BuildBindingParameterPlaceholder(const size_t parameters_count) {
  DCHECK_NE(0UL, parameters_count);

  const std::vector<std::string> placeholders(parameters_count, "?");

  return base::StringPrintf("(%s)",
                            base::JoinString(placeholders, ", ").c_str());
}

std::string BuildBindingParameterPlaceholders(const size_t parameters_count,
                                              const size_t values_count) {
  DCHECK_NE(0UL, values_count);

  const std::string value = BuildBindingParameterPlaceholder(parameters_count);
  if (values_count == 1) {
    return value;
  }

  const std::vector<std::string> values(values_count, value);

  return base::JoinString(values, ", ");
}

void Bind(sql::Statement* statement, const mojom::DBCommandBinding& binding) {
  DCHECK(statement);

  switch (binding.value->which()) {
    case mojom::DBValue::Tag::NULL_VALUE: {
      statement->BindNull(binding.index);
      break;
    }

    case mojom::DBValue::Tag::INT_VALUE: {
      statement->BindInt(binding.index, binding.value->get_int_value());
      break;
    }

    case mojom::DBValue::Tag::INT64_VALUE: {
      statement->BindInt64(binding.index, binding.value->get_int64_value());
      break;
    }

    case mojom::DBValue::Tag::DOUBLE_VALUE: {
      statement->BindDouble(binding.index, binding.value->get_double_value());
      break;
    }

    case mojom::DBValue::Tag::BOOL_VALUE: {
      statement->BindBool(binding.index, binding.value->get_bool_value());
      break;
    }

    case mojom::DBValue::Tag::STRING_VALUE: {
      statement->BindString(binding.index, binding.value->get_string_value());
      break;
    }
  }
}

void BindNull(mojom::DBCommand* command, const int_fast16_t index) {
  DCHECK(command);

  mojom::DBCommandBindingPtr binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::New();
  binding->value->set_null_value(0);

  command->bindings.push_back(std::move(binding));
}

void BindInt(mojom::DBCommand* command, const int index, const int32_t value) {
  DCHECK(command);

  mojom::DBCommandBindingPtr binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::New();
  binding->value->set_int_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindInt64(mojom::DBCommand* command,
               const int index,
               const int64_t value) {
  DCHECK(command);

  mojom::DBCommandBindingPtr binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::New();
  binding->value->set_int64_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindDouble(mojom::DBCommand* command,
                const int index,
                const double value) {
  DCHECK(command);

  mojom::DBCommandBindingPtr binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::New();
  binding->value->set_double_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindBool(mojom::DBCommand* command, const int index, const bool value) {
  DCHECK(command);

  mojom::DBCommandBindingPtr binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::New();
  binding->value->set_bool_value(value);

  command->bindings.push_back(std::move(binding));
}

void BindString(mojom::DBCommand* command,
                const int index,
                const std::string& value) {
  DCHECK(command);

  mojom::DBCommandBindingPtr binding = mojom::DBCommandBinding::New();
  binding->index = index;
  binding->value = mojom::DBValue::New();
  binding->value->set_string_value(value);

  command->bindings.push_back(std::move(binding));
}

}  // namespace database
}  // namespace ads
