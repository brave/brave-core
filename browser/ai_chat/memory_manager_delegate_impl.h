// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_MEMORY_MANAGER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_AI_CHAT_MEMORY_MANAGER_DELEGATE_IMPL_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/memory_manager_delegate.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

// Gives the memory settings of a profile the semantic search over the memories
// (the embeddings service) and the learned memories (the UserMemoryManager of
// the profile's AIChatService). It looks both up each time, so it holds no
// pointer to them.
class MemoryManagerDelegateImpl : public MemoryManagerDelegate {
 public:
  explicit MemoryManagerDelegateImpl(content::BrowserContext* context);
  MemoryManagerDelegateImpl(const MemoryManagerDelegateImpl&) = delete;
  MemoryManagerDelegateImpl& operator=(const MemoryManagerDelegateImpl&) =
      delete;
  ~MemoryManagerDelegateImpl() override;

  // MemoryManagerDelegate:
  void SearchMemories(const std::string& query,
                      SearchMemoriesCallback callback) override;
  void GetLearnedMemories(GetLearnedMemoriesCallback callback) override;
  void DeleteLearnedMemory(const std::string& uuid,
                           DeleteLearnedMemoryCallback callback) override;
  void DreamNow(DreamNowCallback callback) override;

 private:
  raw_ptr<content::BrowserContext> context_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_MEMORY_MANAGER_DELEGATE_IMPL_H_
