// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_NAVIGATION_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_NAVIGATION_TOOL_H_

#include "brave/browser/ai_chat/content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

class NavigationTool : public Tool {
 public:
  NavigationTool(ContentAgentTaskProvider* task_provider,
                 actor::ActorKeyedService* actor_service);
  ~NavigationTool() override;

  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;

  std::optional<std::vector<std::string>> RequiredProperties() const override;

  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  void OnActionsFinished(UseToolCallback callback,
                         optimization_guide::proto::ActionsResult result);
  void OnTabHandleCreated(UseToolCallback callback,
                          GURL url,
                          tabs::TabHandle tab_handle);
  void OnTaskStateReceived(
      UseToolCallback callback,
      std::vector<mojom::ContentBlockPtr> tool_result,
      std::optional<std::vector<mojom::ContentBlockPtr>> task_state);

  raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;
  raw_ptr<ContentAgentTaskProvider> task_provider_ = nullptr;

  base::WeakPtrFactory<NavigationTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_NAVIGATION_TOOL_H_
