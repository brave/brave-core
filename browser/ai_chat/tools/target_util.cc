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
      // Which frame to target, matches actor::DomNode::document_identifier
      // and is usually a value received within a previous tool output that
      // describes the DOM tree.
      {{"document_identifier",
        StringProperty("Document identifier for the target frame")},
       // Which element to target, matches actor::DomNode::node_id
       // If not specified, the root element is specified to target the
       // viewport.
       {"content_node_id", IntegerProperty("DOM node ID of the target element "
                                           "within the frame (optional)")}}));

  any_of->Append(
      ObjectProperty("Screen coordinates of target (less stable)",
                     {{"x", NumberProperty("X coordinate in pixels")},
                      {"y", NumberProperty("Y coordinate in pixels")}}));

  return target_property;
}

base::expected<optimization_guide::proto::ActionTarget, std::string>
ParseTargetInput(const base::Value::Dict& target_dict) {
  // Check what targeting approaches are present
  auto x_value = target_dict.FindDouble("x");
  auto y_value = target_dict.FindDouble("y");
  auto content_node_id = target_dict.FindInt("content_node_id");
  auto* document_identifier = target_dict.FindString("document_identifier");

  // Ensure exactly one approach is used
  // - x and y; or
  // - document_identifier (content_node_id is optional)
  if ((x_value || y_value) && (document_identifier || content_node_id)) {
    return base::unexpected(
        "Target must contain either 'x' and 'y' or "
        "'document_identifier' with optional 'content_node_id', not both");
  }

  // We check content_node_id here even though it's optional so that
  // we can return a more specific error message when checking that case.
  if (!x_value && !y_value && !content_node_id && !document_identifier) {
    return base::unexpected(
        "Target must contain one of either 'x' and 'y' or "
        "'document_identifier' "
        "and optional 'content_node_id'");
  }

  // Parse coordinates approach
  if (x_value || y_value) {
    if (!x_value || !y_value) {
      return base::unexpected(
          "Invalid coordinates: both 'x' and 'y' are required");
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
      return base::unexpected(
          "Invalid identifiers: 'document_identifier' is required when "
          "specifying 'content_node_id'");
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
