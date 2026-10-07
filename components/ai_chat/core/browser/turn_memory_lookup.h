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
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

struct TurnMemoryConfig {
  // Reads the chat time settings from the learned memory feature params.
  static TurnMemoryConfig FromFeatures();

  // The lookup embeds the last user messages, takes the |max_candidates|
  // closest learned memories, and asks the decision model which of them are
  // relevant.
  size_t max_candidates = 12;
  // A candidate needs at least this "yes" probability.
  double relevance_threshold = 0.3;
  // The most learned memories that go in one request.
  size_t max_results = 5;
  // The lookup gives up after this time. The turn then has no relevant learned
  // memories, and the request goes on. The permanent memories still go.
  base::TimeDelta timeout = base::Seconds(3);
  // The longest part of a user message that the embedder reads.
  size_t max_message_length = 1000;
};

// Finds the learned memories for one chat turn (Track B):
//  1. read all learned memories
//  2. the permanent ones always go
//  3. embed the last user messages, at the priority of the user
//  4. take the candidates that are closest to the messages
//  5. ask the decision model if each candidate is relevant, and keep the best
//  6. give the memories text with the date of the last mention
// The lookup never fails: when a step fails or takes too long, the result has
// only the permanent memories.
class TurnMemoryLookup {
 public:
  using DoneCallback =
      base::OnceCallback<void(EngineConsumer::LearnedMemories)>;

  // |data_source|, |decision_client| and |embedder| must outlive the lookup.
  // |user_messages| are the last user messages, the newest first. |done| runs
  // one time, unless the lookup is deleted before.
  TurnMemoryLookup(LearnedMemoryDataSource& data_source,
                   MemoryDecisionClient& decision_client,
                   passage_embeddings::Embedder& embedder,
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
  void OnMemories(std::vector<LearnedMemory> memories);
  void OnEmbedded(std::vector<std::string> passages,
                  std::vector<passage_embeddings::Embedding> embeddings,
                  uint64_t job_id,
                  passage_embeddings::ComputeEmbeddingsStatus status);
  void OnRelevance(std::optional<std::vector<double>> relevance);
  // Runs |done_| with the result so far. The result has relevant memories only
  // after the relevance answer.
  void Finish();

  const raw_ref<LearnedMemoryDataSource> data_source_;
  const raw_ref<MemoryDecisionClient> decision_client_;
  const raw_ref<passage_embeddings::Embedder> embedder_;
  const TurnMemoryConfig config_;
  const std::vector<std::string> user_messages_;
  DoneCallback done_;

  // The learned memories that are not permanent.
  std::vector<LearnedMemory> candidates_;
  EngineConsumer::LearnedMemories result_;
  std::optional<passage_embeddings::Embedder::Job> embed_job_;
  // For the log (--vmodule=turn_memory_lookup=1).
  base::TimeTicks start_time_;
  base::TimeTicks memories_time_;
  base::TimeTicks embedded_time_;
  size_t memory_count_ = 0;
  base::OneShotTimer timeout_timer_;
  base::WeakPtrFactory<TurnMemoryLookup> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TURN_MEMORY_LOOKUP_H_
