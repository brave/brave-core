// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/target_util.h"

#include "base/notreached.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "chrome/common/actor/actor_constants.h"

namespace ai_chat::target_util {

base::Value::Dict TargetProperty(const std::string& description) {
  base::Value::Dict target_property;
  target_property.Set("description", description);
  base::Value::List* any_of = target_property.EnsureList("anyOf");

  any_of->Append(ObjectProperty(
      "DOM element identifiers of target (preferred)",
      {{"document_identifier",
        StringProperty("Document identifier for the target frame")},
       {"content_node_id", IntegerProperty("DOM node ID of the target element "
                                           "within the frame (optional)")}}));

  any_of->Append(
      ObjectProperty("Screen coordinates of target (less stable)",
                     {{"x", NumberProperty("X coordinate in pixels")},
                      {"y", NumberProperty("Y coordinate in pixels")}}));

  return target_property;
}

std::optional<optimization_guide::proto::ActionTarget> ParseTargetInput(
    const base::Value::Dict& target_dict,
    std::string* out_error) {
  // Check what targeting approaches are present
  auto x_value = target_dict.FindDouble("x");
  auto y_value = target_dict.FindDouble("y");
  auto content_node_id = target_dict.FindInt("content_node_id");
  auto* document_identifier = target_dict.FindString("document_identifier");

  // Ensure exactly one approach is used
  if (x_value && y_value && document_identifier) {
    if (out_error) {
      *out_error =
          "Target must contain either 'x' and 'y' or "
          "'document_identifier' with optional 'content_node_id', not both";
    }
    return std::nullopt;
  }

  if (!x_value && !y_value && !content_node_id && !document_identifier) {
    if (out_error) {
      *out_error =
          "Target must contain one of either 'x' and 'y' or "
          "'document_identifier' "
          "and optional 'content_node_id'";
    }
    return std::nullopt;
  }

  // Parse coordinates approach
  if (x_value || y_value) {
    if (!x_value || !y_value) {
      if (out_error) {
        *out_error = "Invalid coordinates: both 'x' and 'y' are required";
      }
      return std::nullopt;
    }

    optimization_guide::proto::ActionTarget target;
    auto* coordinate = target.mutable_coordinate();
    coordinate->set_x(static_cast<int32_t>(x_value.value()));
    coordinate->set_y(static_cast<int32_t>(y_value.value()));
    return target;
  }

  // Parse identifiers approach
  if (content_node_id || document_identifier) {
    if (!document_identifier) {
      if (out_error) {
        *out_error =
            "Invalid identifiers: 'document_identifier' is required when "
            "specifying 'content_node_id'";
      }
      return std::nullopt;
    }

    if (!content_node_id) {
      content_node_id = actor::kRootElementDomNodeId;
    }

    optimization_guide::proto::ActionTarget target;
    target.set_content_node_id(content_node_id.value());
    auto* doc_id = target.mutable_document_identifier();
    doc_id->set_serialized_token(*document_identifier);
    return target;
  }

  // Should never reach here due to earlier checks
  NOTREACHED();
}

}  // namespace ai_chat::target_util
