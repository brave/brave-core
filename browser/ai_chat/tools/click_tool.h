// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_CLICK_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_CLICK_TOOL_H_

#include "base/values.h"
#include "brave/browser/ai_chat/content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"

namespace ai_chat {

class ClickTool : public Tool {
 public:
  ClickTool(ContentAgentTaskProvider* task_provider,
            actor::ActorKeyedService* actor_service);
  ~ClickTool() override;

  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;

  std::optional<std::vector<std::string>> RequiredProperties() const override;

  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnTabHandleCreated(UseToolCallback callback,
                          optimization_guide::proto::ActionTarget target,
                          std::string click_type,
                          std::string click_count,
                          tabs::TabHandle tab_handle);

  raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;
  raw_ptr<ContentAgentTaskProvider> task_provider_ = nullptr;

  base::WeakPtrFactory<ClickTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_CLICK_TOOL_H_
