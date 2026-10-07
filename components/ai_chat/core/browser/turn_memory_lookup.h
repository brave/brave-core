// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TURN_MEMORY_LOOKUP_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TURN_MEMORY_LOOKUP_H_

#include <stddef.h>

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/learned_memory_data_source.h"
#include "brave/components/ai_chat/core/browser/learned_memory_search.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

struct TurnMemoryConfig {
  // Reads the chat time settings from the learned memory feature params.
  static TurnMemoryConfig FromFeatures();

  // The lookup searches the learned memories for the last user messages,
  // takes the |max_candidates| closest ones, and asks the decision model which
  // of them are relevant.
  size_t max_candidates = 12;
  // A candidate needs at least this "yes" probability.
  double relevance_threshold = 0.3;
  // The most learned memories that go in one request.
  size_t max_results = 5;
  // The lookup gives up after this time. The turn then has no relevant learned
  // memories, and the request goes on. The permanent memories still go.
  base::TimeDelta timeout = base::Seconds(3);
  // The longest part of a user message that the search reads.
  size_t max_message_length = 1000;
};

// Finds the learned memories for one chat turn (Track B):
//  1. read the permanent memories: they always go
//  2. search the index for the memories closest to the last user messages
//     (the search embeds the messages at the priority of the user)
//  3. read the found memories from the chat database
//  4. ask the decision model if each candidate is relevant, and keep the best
//  5. give the memories text with the date of the last mention
// Without the search (semantic search is off), only the permanent memories go.
// The lookup never fails: when a step fails or takes too long, the result has
// only the permanent memories.
class TurnMemoryLookup {
 public:
  using DoneCallback =
      base::OnceCallback<void(EngineConsumer::LearnedMemories)>;

  // |data_source| and |decision_client| must outlive the lookup. |search| can
  // be null. |user_messages| are the last user messages, the newest first.
  // |done| runs one time, unless the lookup is deleted before.
  TurnMemoryLookup(LearnedMemoryDataSource& data_source,
                   base::WeakPtr<LearnedMemorySearch> search,
                   MemoryDecisionClient& decision_client,
                   TurnMemoryConfig config,
                   std::vector<std::string> user_messages,
                   DoneCallback done);
  TurnMemoryLookup(const TurnMemoryLookup&) = delete;
  TurnMemoryLookup& operator=(const TurnMemoryLookup&) = delete;
  ~TurnMemoryLookup();

  void Start();

  // The text that goes in the request for a learned memory: the memory and the
  // date of its last mention, for example "Lives in Berlin (2026-08-22)".
  static std::string FormatForPrompt(const LearnedMemory& memory);

 private:
  void OnPermanentMemories(std::vector<LearnedMemory> memories);
  void OnMatches(std::vector<LearnedMemoryMatch> matches);
  void OnCandidates(std::vector<LearnedMemory> memories);
  void OnRelevance(std::optional<std::vector<double>> relevance);
  // Runs |done_| with the result so far. The result has relevant memories only
  // after the relevance answer.
  void Finish();

  const raw_ref<LearnedMemoryDataSource> data_source_;
  base::WeakPtr<LearnedMemorySearch> search_;
  const raw_ref<MemoryDecisionClient> decision_client_;
  const TurnMemoryConfig config_;
  const std::vector<std::string> user_messages_;
  DoneCallback done_;

  // The last user messages that the search and the decision model read, the
  // newest first.
  std::vector<std::string> passages_;
  // The closest learned memories that are not permanent, the closest first.
  std::vector<LearnedMemory> candidates_;
  EngineConsumer::LearnedMemories result_;
  // For the log (--vmodule=turn_memory_lookup=1).
  base::TimeTicks start_time_;
  base::TimeTicks permanent_time_;
  base::TimeTicks searched_time_;
  base::TimeTicks candidates_time_;
  size_t match_count_ = 0;
  base::OneShotTimer timeout_timer_;
  base::WeakPtrFactory<TurnMemoryLookup> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TURN_MEMORY_LOOKUP_H_
