// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_DATA_SOURCE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_DATA_SOURCE_H_

#include <map>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

// The chat data that learned memory (Dreaming, the lookup for a chat turn)
// reads and writes. AIChatService implements it with thin members that call
// its database, the same way it gives chats to other services. Learned memory
// never holds the database itself.
//
// While chat history storage is off, every method gives an empty result or
// false, and the callback runs later (never inside the call).
class LearnedMemoryDataSource {
 public:
  virtual ~LearnedMemoryDataSource() = default;

  // Whether chat history storage is on and the chat database is open.
  virtual bool IsStorageReady() const = 0;

  // The stored chats. Temporary chats are never stored.
  virtual void GetStoredConversations(
      base::OnceCallback<void(std::vector<mojom::ConversationPtr>)>
          callback) = 0;
  // The entries of a stored chat, or null when the chat is not stored.
  virtual void GetStoredConversationData(
      const std::string& conversation_uuid,
      base::OnceCallback<void(mojom::ConversationArchivePtr)> callback) = 0;

  // All learned memories, with their vectors and sources.
  virtual void GetLearnedMemories(
      base::OnceCallback<void(std::vector<LearnedMemory>)> callback) = 0;
  // Adds the memory or replaces the one with the same uuid. A successful write
  // tells the observers of AIChatService (OnLearnedMemoriesChanged).
  virtual void AddOrUpdateLearnedMemory(
      LearnedMemory memory,
      base::OnceCallback<void(bool)> callback) = 0;
  // Deletes the memory for good. A successful delete tells the observers too.
  virtual void DeleteLearnedMemory(const std::string& memory_uuid,
                                   base::OnceCallback<void(bool)> callback) = 0;

  // The date of the last user turn that Dreaming processed, for each chat.
  virtual void GetMemoryWatermarks(
      base::OnceCallback<void(std::map<std::string, base::Time>)> callback) = 0;
  virtual void SetMemoryWatermark(const std::string& conversation_uuid,
                                  base::Time last_processed_entry_date,
                                  base::OnceCallback<void(bool)> callback) = 0;

  // Only for the eval mode (learned_memory_eval.h), which puts chats with old
  // dates in the database. Nothing else calls these.
  virtual void ImportConversationForEval(
      mojom::ConversationPtr conversation,
      mojom::ConversationTurnPtr first_entry) = 0;
  virtual void ImportConversationEntryForEval(
      const std::string& conversation_uuid,
      mojom::ConversationTurnPtr entry) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_DATA_SOURCE_H_
