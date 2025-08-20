// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_UTIL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_UTIL_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat::target_util {

// Creates a standardized "target" property for tool input schemas
// The target property allows either coordinates OR identifiers, never both
// Example usage: {"target": TargetProperty("Element to click on")}
//
// Generates schema like:
// {
//   "type": "object",
//   "description": "Element to click on",
//   "anyOf": [
//     {
//       "type": "object",
//       "properties": {"x": {...}, "y": {...}}
//       }
//     },
//     {
//       "type": "object",
//       "properties": {"content_node_id": {...}, "document_identifier": {...}}
//     }
//   ]
// }
base::Value::Dict TargetProperty(const std::string& description);

// Parse target
// Expects target_dict to be in the format defined in `TargetProperty`.
//
// Returns ActionTarget proto on success, nullopt on parse error
// with the detailed error message in out_error.
std::optional<optimization_guide::proto::ActionTarget> ParseTargetInput(
    const base::Value::Dict& target_dict,
    std::string* out_error = nullptr);

}  // namespace ai_chat::target_util

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_UTIL_H_
