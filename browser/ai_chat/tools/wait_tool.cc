// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/wait_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/common/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameWaitTimeMs[] = "wait_time_ms";

}  // namespace

WaitTool::WaitTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

WaitTool::~WaitTool() = default;

std::string_view WaitTool::Name() const {
  return mojom::kWaitToolName;
}

std::string_view WaitTool::Description() const {
  return "Wait for a specified amount of time before continuing with other "
         "actions. This can be useful to allow pages to load or animations "
         "to complete. Time is specified in milliseconds.";
}

std::optional<base::Value::Dict> WaitTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyNameWaitTimeMs,
        IntegerProperty("The amount of time to wait in milliseconds")}});
}

std::optional<std::vector<std::string>> WaitTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>({kPropertyNameWaitTimeMs});
}

void WaitTool::UseTool(const std::string& input_json,
                       UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"));
    return;
  }

  // Validate wait_time_ms parameter
  std::optional<int> wait_time_ms = input->FindInt(kPropertyNameWaitTimeMs);
  if (!wait_time_ms.has_value() || wait_time_ms.value() <= 0) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: invalid or missing 'wait_time_ms' property. Must be a "
        "non-negative integer higher than 0."));
    return;
  }

  task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
      &WaitTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), wait_time_ms.value()));
}

void WaitTool::OnTabHandleCreated(UseToolCallback callback,
                                  int wait_time_ms,
                                  tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* wait_action = action->mutable_wait();
  wait_action->set_wait_time_ms(wait_time_ms);

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
