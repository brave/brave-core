// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/click_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/browser/ai_chat/content_agent_task_provider.h"
#include "brave/browser/ai_chat/tools/target_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameTarget[] = "target";
constexpr char kPropertyNameClickType[] = "click_type";
constexpr char kPropertyNameClickCount[] = "click_count";
constexpr char kClickTypeLeft[] = "left";
constexpr char kClickTypeRight[] = "right";
constexpr char kClickCountSingle[] = "single";
constexpr char kClickCountDouble[] = "double";

}  // namespace

ClickTool::ClickTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

ClickTool::~ClickTool() = default;

std::string_view ClickTool::Name() const {
  return "click_element";
}

std::string_view ClickTool::Description() const {
  return "Click on an element in the current web page. Use the 'target' "
         "object to specify either DOM element identifiers or screen "
         "coordinates. Supports left/right click and single/double click.";
}

std::optional<base::Value::Dict> ClickTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyNameTarget,
        target_util::TargetProperty("Element to click on")},
       {kPropertyNameClickType,
        StringProperty(
            "Type of click to perform",
            std::vector<std::string>{kClickTypeLeft, kClickTypeRight})},
       {kPropertyNameClickCount,
        StringProperty(
            "Number of clicks to perform",
            std::vector<std::string>{kClickCountSingle, kClickCountDouble})}});
}

std::optional<std::vector<std::string>> ClickTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {kPropertyNameTarget, kPropertyNameClickType, kPropertyNameClickCount});
}

void ClickTool::UseTool(const std::string& input_json,
                        UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json);

  if (!input.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  // Validate click_type and click_count
  const auto* click_type = input->FindString(kPropertyNameClickType);
  const auto* click_count = input->FindString(kPropertyNameClickCount);

  if (!click_type ||
      (*click_type != kClickTypeLeft && *click_type != kClickTypeRight)) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid or missing click_type. Must be 'left' or 'right'."));
    return;
  }

  if (!click_count || (*click_count != kClickCountSingle &&
                       *click_count != kClickCountDouble)) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid or missing click_count. Must be 'single' or 'double'."));
    return;
  }

  // Extract and parse target object
  const base::Value::Dict* target_dict = input->FindDict(kPropertyNameTarget);
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
      &ClickTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(target.value()), std::move(*click_type),
      *click_count));
}

void ClickTool::OnTabHandleCreated(
    UseToolCallback callback,
    optimization_guide::proto::ActionTarget target,
    const std::string& click_type,
    const std::string& click_count,
    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* click_action = action->mutable_click();
  click_action->set_tab_id(tab_handle.raw_value());

  // Set target directly from parsed ActionTarget
  *click_action->mutable_target() = std::move(target);

  // Set click type
  if (click_type == kClickTypeLeft) {
    click_action->set_click_type(optimization_guide::proto::ClickAction::LEFT);
  } else {
    click_action->set_click_type(optimization_guide::proto::ClickAction::RIGHT);
  }

  // Set click count
  if (click_count == kClickCountSingle) {
    click_action->set_click_count(
        optimization_guide::proto::ClickAction::SINGLE);
  } else {
    click_action->set_click_count(
        optimization_guide::proto::ClickAction::DOUBLE);
  }

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
