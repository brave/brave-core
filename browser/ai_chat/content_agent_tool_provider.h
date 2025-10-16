// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TOOL_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TOOL_PROVIDER_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_chat/content_agent_task_provider.h"
#include "brave/browser/ai_chat/content_agent_tool_provider_factory.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "chrome/browser/actor/actor_keyed_service.h"

namespace ai_chat {

// Provides content agent tools to conversations, manages the lifecycle of
// those tools, and provides the actor tasks for the tools to act on as well as
// interfacing with the actor service to execute the actions, deciding
// which tabs to act on.
class ContentAgentToolProvider : public ToolProvider,
                                 public ContentAgentTaskProvider {
 public:
  ContentAgentToolProvider(Profile* profile,
                           actor::ActorKeyedService* actor_service);

  ~ContentAgentToolProvider() override;

  ContentAgentToolProvider(const ContentAgentToolProvider&) = delete;
  ContentAgentToolProvider& operator=(const ContentAgentToolProvider&) = delete;

  // ToolProvider implementation
  std::vector<base::WeakPtr<Tool>> GetTools() override;
  void StopAllTasks() override;

  // ContentAgentTaskProvider implementation
  actor::TaskId GetTaskId() override;
  void GetOrCreateTabHandleForTask(
      base::OnceCallback<void(tabs::TabHandle)> callback) override;
  void ExecuteActions(optimization_guide::proto::Actions actions,
                      Tool::UseToolCallback callback) override;

 private:
  friend class BrowserToolsTest;
  friend class ContentAgentToolProviderTest;
  friend class ContentAgentToolProviderBrowserTest;

  void CreateTools();

  void TabAddedToTask(base::OnceCallback<void(tabs::TabHandle)> callback,
                      actor::mojom::ActionResultPtr result);
  void OnActionsFinished(
      Tool::UseToolCallback callback,
      actor::mojom::ActionResultCode result_code,
      std::optional<size_t> index_of_failed_action,
      std::vector<actor::ActionResultWithLatencyInfo> action_results);
  void ReceivedAnnotatedPageContent(
      Tool::UseToolCallback callback,
      std::optional<optimization_guide::AIPageContentResult> content);

  // Browser-specific tools owned by this provider.
  // Note: if it becomes an advantage to refer directly to a specific tool,
  // then there's no need for a vector - we can simply store each tool in its
  // own member.
  std::vector<std::unique_ptr<Tool>> tools_;

  actor::TaskId task_id_;
  tabs::TabHandle task_tab_handle_;
  raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;

  base::WeakPtrFactory<ContentAgentToolProvider> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TOOL_PROVIDER_H_
