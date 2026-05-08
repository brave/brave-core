// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/mock_tool_provider.h"

#include "base/memory/weak_ptr.h"

namespace ai_chat {

MockToolProvider::MockToolProvider() = default;

MockToolProvider::~MockToolProvider() = default;

std::vector<base::WeakPtr<Tool>> MockToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> result;
  result.reserve(tools_.size());
  for (const auto& tool : tools_) {
    result.push_back(tool->GetWeakPtr());
  }
  return result;
}

void MockToolProvider::StartContentTask(int32_t tab_id) {
  for (auto& observer : observers_) {
    observer.OnContentTaskStarted(tab_id);
  }
}

void MockToolProvider::SetIsPausedByUser(bool is_paused) {
  is_paused_by_user_ = is_paused;
  NotifyTaskStateChanged();
}

bool MockToolProvider::IsPausedByUser() {
  return is_paused_by_user_;
}

}  // namespace ai_chat
