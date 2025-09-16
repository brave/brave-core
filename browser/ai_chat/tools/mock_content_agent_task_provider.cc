// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"

namespace ai_chat {

MockContentAgentTaskProvider::MockContentAgentTaskProvider()
    : task_id_(actor::TaskId(456)) {}  // Default task ID
MockContentAgentTaskProvider::~MockContentAgentTaskProvider() = default;

actor::TaskId MockContentAgentTaskProvider::GetTaskId() {
  return task_id_;
}

}  // namespace ai_chat
