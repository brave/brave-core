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
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameTarget[] = "target";
constexpr char kPropertyNameDirection[] = "direction";
constexpr char kPropertyNameDistance[] = "distance";

constexpr char kDirectionLeft[] = "left";
constexpr char kDirectionRight[] = "right";
constexpr char kDirectionUp[] = "up";
constexpr char kDirectionDown[] = "down";

}  // namespace

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
      {{kPropertyNameTarget,
        target_util::TargetProperty("Document or Element to scroll")},
       {kPropertyNameDirection,
        StringProperty("Direction to scroll",
                       std::vector<std::string>{kDirectionLeft, kDirectionRight,
                                                kDirectionUp, kDirectionDown})},
       {kPropertyNameDistance,
        NumberProperty("Distance to scroll in pixels. It is suggested to use a "
                       "value that will enable an interactive element to be "
                       "used given the element's specified position and the "
                       "specified viewport dimensions and position.")}});
}

std::optional<std::vector<std::string>> ScrollTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {kPropertyNameTarget, kPropertyNameDirection, kPropertyNameDistance});
}

void ScrollTool::UseTool(const std::string& input_json,
                         UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"));
    return;
  }

  // Validate direction and distance
  const auto* direction = input->FindString(kPropertyNameDirection);
  std::optional<double> distance = input->FindDouble(kPropertyNameDistance);

  if (!direction ||
      (*direction != kDirectionLeft && *direction != kDirectionRight &&
       *direction != kDirectionUp && *direction != kDirectionDown)) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: invalid or missing direction. Must be one of: "
        "'left', 'right', 'up', or 'down'."));
    return;
  }

  if (!distance.has_value() || distance.value() <= 0) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: invalid or missing distance. Must be a positive number."));
    return;
  }

  // Extract and parse target object
  const base::Value::Dict* target_dict = input->FindDict(kPropertyNameTarget);
  if (!target_dict) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing 'target' property"));
    return;
  }

  auto target = target_util::ParseTargetInput(*target_dict);
  if (!target.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        base::StrCat({"Invalid 'target': ", target.error()})));
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
  if (direction == kDirectionLeft) {
    scroll_action->set_direction(optimization_guide::proto::ScrollAction::LEFT);
  } else if (direction == kDirectionRight) {
    scroll_action->set_direction(
        optimization_guide::proto::ScrollAction::RIGHT);
  } else if (direction == kDirectionUp) {
    scroll_action->set_direction(optimization_guide::proto::ScrollAction::UP);
  } else {  // down
    scroll_action->set_direction(optimization_guide::proto::ScrollAction::DOWN);
  }

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
