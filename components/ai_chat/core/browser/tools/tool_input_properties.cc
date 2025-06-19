// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"

#include <string>

#include "base/values.h"

namespace ai_chat {

base::Value::Dict StringProperty(
    const std::string& description,
    const std::optional<std::vector<std::string>>& enum_values) {
  base::Value::Dict property;
  property.Set("type", "string");
  if (!description.empty()) {
    property.Set("description", description);
  }

  if (enum_values.has_value() && !enum_values->empty()) {
    base::Value::List enum_list;
    for (const auto& value : enum_values.value()) {
      enum_list.Append(value);
    }
    property.Set("enum", std::move(enum_list));
  }

  return property;
}

base::Value::Dict ArrayProperty(const std::string& description,
                                base::Value::Dict items) {
  base::Value::Dict property;
  property.Set("type", "array");
  if (!description.empty()) {
    property.Set("description", description);
  }
  property.Set("items", std::move(items));

  return property;
}

base::Value::Dict ObjectProperty(
    const std::string& description,
    std::initializer_list<std::pair<const std::string, base::Value::Dict>>
        properties) {
  base::Value::Dict property;
  property.Set("type", "object");
  if (!description.empty()) {
    property.Set("description", description);
  }

  base::Value::Dict properties_dict;
  for (auto& [key, value] : properties) {
    properties_dict.Set(key, value.Clone());
  }
  property.Set("properties", std::move(properties_dict));

  return property;
}

base::Value::Dict BooleanProperty(const std::string& description) {
  base::Value::Dict property;
  property.Set("type", "boolean");
  if (!description.empty()) {
    property.Set("description", description);
  }

  return property;
}

base::Value::Dict NumberProperty(const std::string& description) {
  base::Value::Dict property;
  property.Set("type", "number");
  if (!description.empty()) {
    property.Set("description", description);
  }

  return property;
}

base::Value::Dict IntegerProperty(const std::string& description) {
  base::Value::Dict property;
  property.Set("type", "integer");
  if (!description.empty()) {
    property.Set("description", description);
  }

  return property;
}

}  // namespace ai_chat
