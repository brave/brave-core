// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/history_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/common/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameDirection[] = "direction";
constexpr char kDirectionBack[] = "back";
constexpr char kDirectionForward[] = "forward";

}  // namespace

HistoryTool::HistoryTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

HistoryTool::~HistoryTool() = default;

std::string_view HistoryTool::Name() const {
  return "navigate_history";
}

std::string_view HistoryTool::Description() const {
  return "Navigate the browser history by going back or forward in the "
         "current tab's session history. This is equivalent to clicking "
         "the browser's back or forward buttons.";
}

std::optional<base::Value::Dict> HistoryTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyNameDirection,
        StringProperty(
            "Direction to navigate in history",
            std::vector<std::string>{kDirectionBack, kDirectionForward})}});
}

std::optional<std::vector<std::string>> HistoryTool::RequiredProperties()
    const {
  return std::optional<std::vector<std::string>>({kPropertyNameDirection});
}

void HistoryTool::UseTool(const std::string& input_json,
                          UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"));
    return;
  }

  // Validate direction parameter
  const auto* direction = input->FindString(kPropertyNameDirection);
  if (!direction ||
      (*direction != kDirectionBack && *direction != kDirectionForward)) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: invalid or missing direction. Must be 'back' or 'forward'."));
    return;
  }

  task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
      &HistoryTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), std::move(*direction)));
}

void HistoryTool::OnTabHandleCreated(UseToolCallback callback,
                                     const std::string& direction,
                                     tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  if (direction == kDirectionBack) {
    auto* back_action = action->mutable_back();
    back_action->set_tab_id(tab_handle.raw_value());
  } else {  // forward
    auto* forward_action = action->mutable_forward();
    forward_action->set_tab_id(tab_handle.raw_value());
  }

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
