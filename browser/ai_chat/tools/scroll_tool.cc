// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/scroll_tool.h"

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

ScrollTool::ScrollTool(BrowserToolTaskProvider* task_provider,
                       actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), task_provider_(task_provider) {}

ScrollTool::~ScrollTool() = default;

std::string_view ScrollTool::Name() const {
  return "scroll_element";
}

std::string_view ScrollTool::Description() const {
  return "Scroll an element or the viewport in the current web page. You can "
         "specify either a content node ID with document identifier, or x/y "
         "coordinates to identify the scrollable element. If scrolling the "
         "viewport, provide the document identifier otherwise scroll will not "
         "occur.";
}

std::optional<base::Value::Dict> ScrollTool::InputProperties() const {
  return CreateInputProperties(
      {{"content_node_id",
        IntegerProperty(
            "The DOM node ID of the element to scroll. Use this "
            "OR coordinates, not both. Use 0 to scroll the viewport.")},
       {"document_identifier",
        StringProperty("Document identifier for the content node")},
       {"x", NumberProperty("X coordinate of scrollable element (if not using "
                            "content_node_id)")},
       {"y", NumberProperty("Y coordinate of scrollable element (if not using "
                            "content_node_id)")},
       {"direction", StringProperty("Direction to scroll",
                                    std::vector<std::string>{"left", "right",
                                                             "up", "down"})},
       {"distance", NumberProperty("Distance to scroll in pixels")}});
}

std::optional<std::vector<std::string>> ScrollTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {"direction", "distance", "document_identifier"});
}

void ScrollTool::UseTool(const std::string& input_json,
                         UseToolCallback callback,
                         std::optional<base::Value> client_data) {
  data_decoder::DataDecoder::ParseJsonIsolated(
      input_json,
      base::BindOnce(&ScrollTool::OnParseJson, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void ScrollTool::OnParseJson(UseToolCallback callback,
                             data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value() || !result->is_dict()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  const auto& dict = result->GetDict();

  // Validate direction and distance
  const auto* direction = dict.FindString("direction");
  std::optional<double> distance = dict.FindDouble("distance");

  if (!direction || (*direction != "left" && *direction != "right" &&
                     *direction != "up" && *direction != "down")) {
    std::move(callback).Run(
        CreateContentBlocksForText("Invalid or missing direction. Must be "
                                   "'left', 'right', 'up', or 'down'."));
    return;
  }

  if (!distance.has_value() || distance.value() <= 0) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid or missing distance. Must be a positive number."));
    return;
  }

  // Check if using content_node_id approach
  std::optional<int> content_node_id = dict.FindInt("content_node_id");
  const auto* document_identifier = dict.FindString("document_identifier");

  // Check if using coordinate approach
  std::optional<double> x = dict.FindDouble("x");
  std::optional<double> y = dict.FindDouble("y");

  if (!document_identifier) {
    std::move(callback).Run(
        CreateContentBlocksForText("document_identifier is required"));
    return;
  }

  if (content_node_id.has_value()) {
    // Using content node approach
    task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
        &ScrollTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
        std::move(callback), content_node_id.value(), *document_identifier, 0.0,
        0.0, *direction, static_cast<float>(distance.value())));
  } else if (x.has_value() && y.has_value()) {
    // Using coordinate approach
    task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
        &ScrollTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
        std::move(callback), -1, *document_identifier, x.value(), y.value(),
        *direction, static_cast<float>(distance.value())));
  } else {
    std::move(callback).Run(CreateContentBlocksForText(
        "Either content_node_id with document_identifier OR x,y coordinates "
        "must be provided."));
    return;
  }
}

void ScrollTool::OnTabHandleCreated(UseToolCallback callback,
                                    int content_node_id,
                                    std::string document_identifier,
                                    double x,
                                    double y,
                                    std::string direction,
                                    float distance,
                                    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* scroll_action = action->mutable_scroll();
  scroll_action->set_tab_id(tab_handle.raw_value());
  scroll_action->set_distance(distance);

  // Set target
  auto* target = scroll_action->mutable_target();
  auto* doc_id = target->mutable_document_identifier();
  doc_id->set_serialized_token(document_identifier);

  if (content_node_id >= 0) {
    // Using content node
    target->set_content_node_id(content_node_id);
  } else if (x != 0.0 || y != 0.0) {
    // Using coordinates
    auto* coordinate = target->mutable_coordinate();
    coordinate->set_x(static_cast<int32_t>(x));
    coordinate->set_y(static_cast<int32_t>(y));
  }

  // Set scroll direction
  if (direction == "left") {
    scroll_action->set_direction(optimization_guide::proto::ScrollAction::LEFT);
  } else if (direction == "right") {
    scroll_action->set_direction(
        optimization_guide::proto::ScrollAction::RIGHT);
  } else if (direction == "up") {
    scroll_action->set_direction(optimization_guide::proto::ScrollAction::UP);
  } else {  // down
    scroll_action->set_direction(optimization_guide::proto::ScrollAction::DOWN);
  }

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
