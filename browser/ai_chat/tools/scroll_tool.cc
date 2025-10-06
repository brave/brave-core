// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/scroll_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/browser/ai_chat/tools/target_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

ScrollTool::ScrollTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

ScrollTool::~ScrollTool() = default;

std::string_view ScrollTool::Name() const {
  return "scroll_element";
}

std::string_view ScrollTool::Description() const {
  return "Scroll an element or the viewport in the current web page. Use the "
         "'target' object to specify either DOM element identifiers or screen "
         "coordinates to identify the scrollable element. This tool should be "
         "used to ensure elements that need to be interacted with are in the "
         "viewport.";
}

std::optional<base::Value::Dict> ScrollTool::InputProperties() const {
  return CreateInputProperties(
      {{"target", target_util::TargetProperty("Document or Element to scroll")},
       {"direction", StringProperty("Direction to scroll",
                                    std::vector<std::string>{"left", "right",
                                                             "up", "down"})},
       {"distance",
        NumberProperty("Distance to scroll in pixels. It is suggested to use a "
                       "value that will enable an interactive element to be "
                       "used given the element's specified position and the "
                       "specified viewport dimensions and position.")}});
}

std::optional<std::vector<std::string>> ScrollTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {"target", "direction", "distance"});
}

void ScrollTool::UseTool(const std::string& input_json,
                         UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json);

  if (!input.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  // Validate direction and distance
  const auto* direction = input->FindString("direction");
  std::optional<double> distance = input->FindDouble("distance");

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

  // Extract and parse target object
  const base::Value::Dict* target_dict = input->FindDict("target");
  if (!target_dict) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing 'target' object"));
    return;
  }

  auto target = target_util::ParseTargetInput(*target_dict);
  if (!target.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(target.error()));
    return;
  }

  task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
      &ScrollTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(target.value()), std::move(*direction),
      static_cast<float>(distance.value())));
}

void ScrollTool::OnTabHandleCreated(
    UseToolCallback callback,
    optimization_guide::proto::ActionTarget target,
    const std::string& direction,
    float distance,
    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* scroll_action = action->mutable_scroll();
  scroll_action->set_tab_id(tab_handle.raw_value());
  scroll_action->set_distance(distance);

  // Set target directly from parsed ActionTarget
  *scroll_action->mutable_target() = std::move(target);

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
