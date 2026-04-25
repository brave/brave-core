// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"

#include <string>
#include <utility>

#include "base/values.h"

namespace ai_chat {

base::DictValue StringProperty(
    const std::string& description,
    const std::optional<std::vector<std::string>>& enum_values) {
  base::DictValue property;
  property.Set("type", "string");
  if (!description.empty()) {
    property.Set("description", description);
  }

  if (enum_values.has_value() && !enum_values->empty()) {
    base::ListValue enum_list;
    for (const auto& value : enum_values.value()) {
      enum_list.Append(value);
    }
    property.Set("enum", std::move(enum_list));
  }

  return property;
}

base::DictValue ArrayProperty(const std::string& description,
                              base::DictValue items) {
  base::DictValue property;
  property.Set("type", "array");
  if (!description.empty()) {
    property.Set("description", description);
  }
  property.Set("items", std::move(items));

  return property;
}

base::DictValue ObjectProperty(
    const std::string& description,
    std::initializer_list<std::pair<const std::string, base::DictValue>>
        properties) {
  base::DictValue property;
  property.Set("type", "object");
  if (!description.empty()) {
    property.Set("description", description);
  }

  base::DictValue properties_dict;
  for (auto& [key, value] : properties) {
    properties_dict.Set(key, value.Clone());
  }
  property.Set("properties", std::move(properties_dict));

  return property;
}

base::DictValue BooleanProperty(const std::string& description) {
  base::DictValue property;
  property.Set("type", "boolean");
  if (!description.empty()) {
    property.Set("description", description);
  }

  return property;
}

base::DictValue NumberProperty(const std::string& description) {
  base::DictValue property;
  property.Set("type", "number");
  if (!description.empty()) {
    property.Set("description", description);
  }

  return property;
}

base::DictValue IntegerProperty(const std::string& description) {
  base::DictValue property;
  property.Set("type", "integer");
  if (!description.empty()) {
    property.Set("description", description);
  }

  return property;
}

base::DictValue CreateInputProperties(
    std::initializer_list<std::pair<const std::string, base::DictValue>>
        properties) {
  base::DictValue input_properties;
  for (auto& [key, value] : properties) {
    input_properties.Set(key, value.Clone());
  }
  return input_properties;
}

}  // namespace ai_chat
