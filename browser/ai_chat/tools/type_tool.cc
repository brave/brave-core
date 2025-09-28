// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/type_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/browser/ai_chat/tools/target_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

TypeTool::TypeTool(ContentAgentTaskProvider* task_provider,
                   actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), task_provider_(task_provider) {}

TypeTool::~TypeTool() = default;

std::string_view TypeTool::Name() const {
  return "type_text";
}

std::string_view TypeTool::Description() const {
  return "Type keyboard characters into an input field on the current web "
         "page. Element must "
         "be editable and focusable. Prefer input over container elements. "
         "Use the 'target' object to specify either DOM element identifiers "
         "or screen coordinates. Supports different modes for handling "
         "existing text and can optionally press Enter after typing. Only "
         "supports a series of ascii characters and no newline characters. If "
         "requiring excplicit new lines, and the element type supports new "
         "lines, break each line up in to a separate tool action and specify "
         "to press enter after each one. The field does not need to "
         "be clicked first as that will be done automatically. For example: [\n"
         "{ name: \"type_text\", target: { ... }, text: \"This is the first "
         "paragraph of text without a newline\", follow_by_enter: true },\n"
         "{ name: \"type_text\", target: { ... }, text: \"And this is the next "
         "paragraph of text without a newline\", follow_by_enter: false },\n"
         "]";
}

std::optional<base::Value::Dict> TypeTool::InputProperties() const {
  return CreateInputProperties(
      {{"target", target_util::TargetProperty("Element to type into")},
       {"text",
        StringProperty("A single line of text: a string of keyboard ascii "
                       "characters to press "
                       "in sequence after the field is clicked. CANNOT INCLUDE "
                       "MULTIPLE LINES OR NEW LINE CHARACTERS!")},
       {"follow_by_enter",
        BooleanProperty("Whether to press Enter after typing the text")},
       {"mode",
        StringProperty(
            "How to handle existing text in the element. Prefer \"append\" for "
            "fields with no existing text.",
            std::vector<std::string>{"replace", "prepend", "append"})}});
}

std::optional<std::vector<std::string>> TypeTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {"target", "text", "follow_by_enter", "mode"});
}

void TypeTool::UseTool(const std::string& input_json,
                       UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json);

  if (!input.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  // Validate required fields
  const auto* text = input->FindString("text");
  std::optional<bool> follow_by_enter = input->FindBool("follow_by_enter");
  const auto* mode = input->FindString("mode");

  if (!text) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required field: text"));
    return;
  }

  if (!follow_by_enter.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required field: follow_by_enter"));
    return;
  }

  if (!mode ||
      (*mode != "replace" && *mode != "prepend" && *mode != "append")) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid or missing mode. Must be 'replace', 'prepend', or 'append'."));
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
      &TypeTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(target.value()), *text,
      follow_by_enter.value(), *mode));
}

void TypeTool::OnTabHandleCreated(
    UseToolCallback callback,
    optimization_guide::proto::ActionTarget target,
    const std::string& text,
    bool follow_by_enter,
    const std::string& mode,
    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* type_action = action->mutable_type();
  type_action->set_tab_id(tab_handle.raw_value());

  // Set target directly from parsed ActionTarget
  *type_action->mutable_target() = std::move(target);

  // Set text content
  type_action->set_text(text);
  type_action->set_follow_by_enter(follow_by_enter);

  // Set type mode
  if (mode == "replace") {
    type_action->set_mode(
        optimization_guide::proto::TypeAction::DELETE_EXISTING);
  } else if (mode == "prepend") {
    type_action->set_mode(optimization_guide::proto::TypeAction::PREPEND);
  } else if (mode == "append") {
    type_action->set_mode(optimization_guide::proto::TypeAction::APPEND);
  }

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
