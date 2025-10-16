// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_MOCK_CONTENT_AGENT_TASK_PROVIDER_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_MOCK_CONTENT_AGENT_TASK_PROVIDER_H_

#include "base/functional/callback.h"
#include "brave/browser/ai_chat/content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/common/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockContentAgentTaskProvider : public ContentAgentTaskProvider {
 public:
  MockContentAgentTaskProvider();
  ~MockContentAgentTaskProvider() override;

  // Override GetTaskId to return a default - no need to mock this boilerplate
  actor::TaskId GetTaskId() override;

  MOCK_METHOD(void,
              GetOrCreateTabHandleForTask,
              (base::OnceCallback<void(tabs::TabHandle)> callback),
              (override));

  MOCK_METHOD(void,
              ExecuteActions,
              (optimization_guide::proto::Actions actions,
               Tool::UseToolCallback callback),
              (override));

  // Allow tests to customize the task ID if needed
  void SetTaskId(actor::TaskId task_id) { task_id_ = task_id; }

 private:
  actor::TaskId task_id_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_MOCK_CONTENT_AGENT_TASK_PROVIDER_H_
