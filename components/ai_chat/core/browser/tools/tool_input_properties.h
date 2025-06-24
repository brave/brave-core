// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_INPUT_PROPERTIES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_INPUT_PROPERTIES_H_

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "base/values.h"

namespace ai_chat {

// Helper functions for building JSON schema properties as base::Value::Dict
// objects for use in Tool::InputProperties()

// Creates a string property with optional enum values
// Example: StringProperty("Location for weather")
// Example: StringProperty("Temperature unit", {"celsius", "fahrenheit"})
base::Value::Dict StringProperty(
    const std::string& description,
    const std::optional<std::vector<std::string>>& enum_values = std::nullopt);

// Creates an array property with items schema
// Example: ArrayProperty("List of locations", StringProperty("A location"))
base::Value::Dict ArrayProperty(const std::string& description,
                                base::Value::Dict items);

// Creates an object property with nested properties
// Example: ObjectProperty("Configuration", {{"enabled", BooleanProperty("Enable
// feature")}})
base::Value::Dict ObjectProperty(
    const std::string& description,
    std::initializer_list<std::pair<const std::string, base::Value::Dict>>
        properties);

base::Value::Dict BooleanProperty(const std::string& description);

// Create a property for either integer or floating-point numbers
base::Value::Dict NumberProperty(const std::string& description);

// Create a property for integer (non floating-point) values
base::Value::Dict IntegerProperty(const std::string& description);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_INPUT_PROPERTIES_H_
