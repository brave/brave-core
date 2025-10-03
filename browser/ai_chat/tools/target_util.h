// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_UTIL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_UTIL_H_

#include <string>

#include "base/types/expected.h"
#include "base/values.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat::target_util {

// Creates a standardized "target" property for tool input schemas.
// The target property allows either coordinates OR identifiers, never both.
// The identifiers are both the document identifier, which identifies which
// frame in a WebContents to target, and an optional node id within that frame.
// The coordinates are x,y values in css pixels from the top-left of the
// viewport.
//
// Example usage: {"target": TargetProperty("Element to click on")}
//
// Generates the following:
// {
//   "target": {
//     "description": "Element to click on",
//     "anyOf": [
//       {
//         "description": "DOM element identifiers of target (preferred)",
//         "properties": {
//           "content_node_id": {
//             "description": "DOM node ID of the target element within the
//             frame (optional)", "type": "integer"
//           },
//           "document_identifier": {
//             "description": "Document identifier for the target frame",
//             "type": "string"
//           }
//         },
//         "type": "object"
//       },
//       {
//         "description": "Screen coordinates of target (less stable)",
//         "properties": {
//           "x": {
//             "description": "X coordinate in pixels",
//             "type": "number"
//           },
//           "y": {
//             "description": "Y coordinate in pixels",
//             "type": "number"
//           }
//         },
//         "type": "object"
//       }
//     ]
//   }
// }
base::Value::Dict TargetProperty(const std::string& description);

// Parse target
// Expects target_dict to be in the format defined in `TargetProperty`.
//
// Returns ActionTarget proto on success, or error string on parse error.
base::expected<optimization_guide::proto::ActionTarget, std::string>
ParseTargetInput(const base::Value::Dict& target_dict);

}  // namespace ai_chat::target_util

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_TARGET_UTIL_H_
