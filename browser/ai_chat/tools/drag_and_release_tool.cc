// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/drag_and_release_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "brave/browser/ai_chat/tools/target_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameFrom[] = "from";
constexpr char kPropertyNameTo[] = "to";

}  // namespace

DragAndReleaseTool::DragAndReleaseTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

DragAndReleaseTool::~DragAndReleaseTool() = default;

std::string_view DragAndReleaseTool::Name() const {
  return "drag_and_release";
}

std::string_view DragAndReleaseTool::Description() const {
  return "Perform a drag and drop operation from one location to another in "
         "the current web page. You can specify either content node IDs with "
         "document identifiers, or x/y coordinates for both the source and "
         "destination locations.";
}

std::optional<base::Value::Dict> DragAndReleaseTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyNameFrom,
        target_util::TargetProperty("Source element to drag from")},
       {kPropertyNameTo,
        target_util::TargetProperty("Target element to drag to")}});
}

std::optional<std::vector<std::string>> DragAndReleaseTool::RequiredProperties()
    const {
  return std::optional<std::vector<std::string>>(
      {kPropertyNameFrom, kPropertyNameTo});
}

void DragAndReleaseTool::UseTool(const std::string& input_json,
                                 UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json);

  if (!input.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: failed to parse input JSON. Please try again."));
    return;
  }

  // Extract and parse source (from) target
  const base::Value::Dict* from_dict = input->FindDict(kPropertyNameFrom);
  if (!from_dict) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing 'from' target object"));
    return;
  }

  auto from_target = target_util::ParseTargetInput(*from_dict);
  if (!from_target.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        base::StrCat({"Invalid 'from' target: ", from_target.error()})));
    return;
  }

  // Extract and parse destination (to) target
  const base::Value::Dict* to_dict = input->FindDict(kPropertyNameTo);
  if (!to_dict) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing 'to' target object"));
    return;
  }

  auto to_target = target_util::ParseTargetInput(*to_dict);
  if (!to_target.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        base::StrCat({"Invalid 'to' target: ", to_target.error()})));
    return;
  }

  task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
      &DragAndReleaseTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(*from_target), std::move(*to_target)));
}

void DragAndReleaseTool::OnTabHandleCreated(
    UseToolCallback callback,
    optimization_guide::proto::ActionTarget from_target,
    optimization_guide::proto::ActionTarget to_target,
    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* drag_action = action->mutable_drag_and_release();
  drag_action->set_tab_id(tab_handle.raw_value());

  // Set targets directly from the parsed ActionTarget objects
  *drag_action->mutable_from_target() = std::move(from_target);
  *drag_action->mutable_to_target() = std::move(to_target);

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
