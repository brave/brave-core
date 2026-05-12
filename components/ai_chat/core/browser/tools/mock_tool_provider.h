// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_PROVIDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_PROVIDER_H_

#include <concepts>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

// A ToolProvider for tests. Tools added via AddToolForTesting() are owned by
// the provider and returned from GetTools(), so tests don't need to wire up
// any mock expectations for the common case.
class MockToolProvider : public ToolProvider {
 public:
  MockToolProvider();
  ~MockToolProvider() override;

  MOCK_METHOD(void, OnGenerationCompleteWithNoToolsToHandle, (), (override));
  MOCK_METHOD(void, StopAllTasks, (), (override));

  // Returns weak pointers to all tools added via AddToolForTesting.
  std::vector<base::WeakPtr<Tool>> GetTools() override;

  // Adds a tool that this provider owns. Returns a pointer to the added tool;
  // ownership stays with the provider and the pointer is valid for the
  // provider's lifetime.
  template <typename T>
    requires(std::derived_from<T, Tool>)
  T* AddToolForTesting(std::unique_ptr<T> tool) {
    T* raw = tool.get();
    tools_.push_back(std::move(tool));
    return raw;
  }

  // Notifies observers that a content task has started on `tab_id`.
  void StartContentTask(int32_t tab_id);

  void SetIsPausedByUser(bool is_paused);
  bool IsPausedByUser() override;

 private:
  std::vector<std::unique_ptr<Tool>> tools_;
  bool is_paused_by_user_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_PROVIDER_H_
