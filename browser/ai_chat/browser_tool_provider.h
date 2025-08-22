// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "brave/browser/ai_chat/browser_tool_provider_factory.h"

namespace ai_chat {

// Implementation of ToolProvider that provides browser-specific
// tools for conversations.
// It is responsible for grouping browser action tasks (a set of tabs)
// that the tools for a conversation perform actions on.
class BrowserToolProvider : public ToolProvider,
                            public BrowserToolTaskProvider {
 public:
  BrowserToolProvider(Profile* profile,
                               raw_ptr<actor::ActorKeyedService> actor_service);

  ~BrowserToolProvider() override;

  BrowserToolProvider(const BrowserToolProvider&) = delete;
  BrowserToolProvider& operator=(const BrowserToolProvider&) = delete;

  // ToolProvider implementation
  std::vector<base::WeakPtr<Tool>> GetTools() override;
  void StopAllTasks() override;

  // BrowserToolTaskProvider implementation
  actor::TaskId GetTaskId() override;
  void GetOrCreateTabHandleForTask(
      base::OnceCallback<void(tabs::TabHandle)> callback) override;
  void ExecuteActions(optimization_guide::proto::Actions actions,
                      Tool::UseToolCallback callback) override;

 private:
  void CreateTools();

  void TabAdded(tabs::TabHandle tab_handle,
    base::OnceCallback<void(tabs::TabHandle)> callback,
    actor::mojom::ActionResultPtr result);
  void OnActionsFinished(Tool::UseToolCallback callback,
    actor::mojom::ActionResultCode result_code,
    std::optional<size_t> index_of_failed_action);
  void ReceivedAnnotatedPageContent(
    Tool::UseToolCallback callback,
    std::optional<optimization_guide::AIPageContentResult> content);

    // Browser-specific tools owned by this provider
    std::vector<std::unique_ptr<Tool>> tools_;
    actor::TaskId task_id_;
    tabs::TabHandle task_tab_handle_;
    raw_ptr<actor::ActorKeyedService> actor_service_ = nullptr;
    raw_ptr<Profile> profile_ = nullptr;

    base::WeakPtrFactory<BrowserToolProvider> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_H_
