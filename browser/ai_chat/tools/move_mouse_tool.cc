// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/move_mouse_tool.h"

#include "base/functional/bind.h"
#include "brave/browser/ai_chat/tools/target_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

MoveMouseTool::MoveMouseTool(ContentAgentTaskProvider* task_provider,
                             actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), task_provider_(task_provider) {}

MoveMouseTool::~MoveMouseTool() = default;

std::string_view MoveMouseTool::Name() const {
  return "move_mouse";
}

std::string_view MoveMouseTool::Description() const {
  return "Move the mouse pointer to a specific location in the current web "
         "page. You can specify either a content node ID with document "
         "identifier, or x/y coordinates.";
}

std::optional<base::Value::Dict> MoveMouseTool::InputProperties() const {
  return CreateInputProperties(
      {{"target", target_util::TargetProperty("Element to move mouse to")}});
}

std::optional<std::vector<std::string>> MoveMouseTool::RequiredProperties()
    const {
  return std::optional<std::vector<std::string>>({"target"});
}

void MoveMouseTool::UseTool(const std::string& input_json,
                            UseToolCallback callback,
                            std::optional<base::Value> client_data) {
  data_decoder::DataDecoder::ParseJsonIsolated(
      input_json,
      base::BindOnce(&MoveMouseTool::OnParseJson,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MoveMouseTool::OnParseJson(
    UseToolCallback callback,
    data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value() || !result->is_dict()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  const auto& dict = result->GetDict();

  // Extract and parse target object
  const base::Value::Dict* target_dict = dict.FindDict("target");
  if (!target_dict) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Either content_node_id with document_identifier OR x,y coordinates "
        "must be provided."));
    return;
  }

  std::string target_error;
  auto target = target_util::ParseTargetInput(*target_dict, &target_error);
  if (!target.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(target_error));
    return;
  }

  task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
      &MoveMouseTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(target.value())));
}

void MoveMouseTool::OnTabHandleCreated(
    UseToolCallback callback,
    optimization_guide::proto::ActionTarget target,
    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* move_mouse_action = action->mutable_move_mouse();
  move_mouse_action->set_tab_id(tab_handle.raw_value());

  // Set target directly from parsed ActionTarget
  *move_mouse_action->mutable_target() = std::move(target);

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
