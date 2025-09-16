// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_HISTORY_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_HISTORY_TOOL_H_

#include "brave/browser/ai_chat/content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

class HistoryTool : public Tool {
 public:
  HistoryTool(ContentAgentTaskProvider* task_provider,
              actor::ActorKeyedService* actor_service);
  ~HistoryTool() override;

  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;

  std::optional<std::vector<std::string>> RequiredProperties() const override;

  void UseTool(const std::string& input_json,
               UseToolCallback callback,
               std::optional<base::Value> client_data) override;

 private:
  void OnParseJson(UseToolCallback callback,
                   data_decoder::DataDecoder::ValueOrError result);
  void OnTabHandleCreated(UseToolCallback callback,
                          std::string direction,
                          tabs::TabHandle tab_handle);

  raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;
  raw_ptr<ContentAgentTaskProvider> task_provider_ = nullptr;

  base::WeakPtrFactory<HistoryTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_HISTORY_TOOL_H_
