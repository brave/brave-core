// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TASK_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TASK_PROVIDER_H_

#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"

namespace ai_chat {

// Provides glue between tools and the actor framework, specifically around
// actor tasks and tool execution. A task provider may choose how to execute
// the actions and provide the results of those actions. This could be
// implemented on platforms that don't support the chromium actor framework.
// Platforms need to provide:
// - implementations for each Action type.
// - TaskId management
// - TabHandle creation
class ContentAgentTaskProvider {
 public:
  virtual ~ContentAgentTaskProvider() = default;

  // Get the task ID for this instance
  virtual actor::TaskId GetTaskId() = 0;

  // Get the current tab for the task.
  // TODO(https://github.com/brave/brave-browser/issues/49258): re-architect
  // so that multiple tab can be added to the task, observed and acted on. The
  // AI can choose which tab to act on via a tab ID parameter, as the actor
  // framework expects.
  virtual void GetOrCreateTabHandleForTask(
      base::OnceCallback<void(tabs::TabHandle)> callback) = 0;

  // Execute the specified actions on their specified tab(s). The tabs must
  // be added to the task prior to calling this method.
  // TODO(https://github.com/brave/brave-browser/issues/49289): Now that we can
  // send ToolRequest directly to the ActorKeyedService, this method can accept
  // those instead of the Actions proto. It will be nicer for the Tools to build
  // the tool requests directly instead of dealing with the proto
  // intermediaries.
  virtual void ExecuteActions(optimization_guide::proto::Actions actions,
                              Tool::UseToolCallback callback) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CONTENT_AGENT_TASK_PROVIDER_H_
