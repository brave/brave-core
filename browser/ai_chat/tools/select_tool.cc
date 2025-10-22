// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/select_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/browser/ai_chat/tools/target_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/common/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameTarget[] = "target";
constexpr char kPropertyNameValue[] = "value";

}  // namespace

SelectTool::SelectTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

SelectTool::~SelectTool() = default;

std::string_view SelectTool::Name() const {
  return mojom::kSelectDropdownToolName;
}

std::string_view SelectTool::Description() const {
  return "Select an option from a dropdown menu (<select> element) in the "
         "current web page. Use the 'target' object to specify either DOM "
         "element identifiers or screen coordinates to identify the dropdown. "
         "The value should match the 'value' attribute of the desired option.";
}

std::optional<base::Value::Dict> SelectTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyNameTarget,
        target_util::TargetProperty("Dropdown element to select from")},
       {kPropertyNameValue,
        StringProperty("The value attribute of the <option> to select")}});
}

std::optional<std::vector<std::string>> SelectTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {kPropertyNameTarget, kPropertyNameValue});
}

void SelectTool::UseTool(const std::string& input_json,
                         UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"));
    return;
  }

  // Validate value parameter
  const auto* value = input->FindString(kPropertyNameValue);
  if (!value) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing required 'value' property"));
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
      &SelectTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(target.value()), std::move(*value)));
}

void SelectTool::OnTabHandleCreated(
    UseToolCallback callback,
    optimization_guide::proto::ActionTarget target,
    const std::string& value,
    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* select_action = action->mutable_select();
  select_action->set_tab_id(tab_handle.raw_value());
  select_action->set_value(value);

  // Set target directly from parsed ActionTarget
  *select_action->mutable_target() = std::move(target);

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
