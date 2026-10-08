// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_DREAMING_RUN_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_DREAMING_RUN_H_

#include <stddef.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/learned_memory_data_source.h"
#include "brave/components/ai_chat/core/browser/learned_memory_search.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/ai_chat/core/browser/memory_llm_prompts.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

enum class DreamingStatus {
  // The run processed all new user turns.
  kCompleted,
  // The run stopped at the time limit. The next run continues.
  kTimedOut,
  // A request to a local model or to chat storage failed, so the run
  // stopped. The next run continues.
  kFailed,
  // The chats were deleted or storage was turned off during the run.
  kCanceled,
  // Another run is active.
  kBusy,
  // Learned memory is not available, for example storage is off.
  kUnavailable,
  // The changes of the last run wait for the user's review.
  kReviewPending,
};

// A change that a run wants to make, for the user to review.
struct DreamingProposal {
  enum class Kind {
    // A memory that the run learned.
    kNew,
    // A stored memory with a new text: a replace or a merge.
    kChanged,
    // A stored memory that the run saw again. Only its dates and sources move.
    kSeenAgain,
  };

  DreamingProposal();
  DreamingProposal(const DreamingProposal&);
  DreamingProposal& operator=(const DreamingProposal&);
  DreamingProposal(DreamingProposal&&);
  DreamingProposal& operator=(DreamingProposal&&);
  ~DreamingProposal();

  Kind kind = Kind::kNew;
  // The memory as the run would store it.
  LearnedMemory memory;
  // The stored memory at the start of the run. Null for kNew.
  std::optional<LearnedMemory> stored;
};

// What a run learned, held until the user reviews it. Nothing is stored before
// that, and the watermarks stay, so a review that is lost (for example at a
// restart) is made again by the next run.
struct DreamingReview {
  DreamingReview();
  DreamingReview(const DreamingReview&) = delete;
  DreamingReview& operator=(const DreamingReview&) = delete;
  DreamingReview(DreamingReview&&);
  DreamingReview& operator=(DreamingReview&&);
  ~DreamingReview();

  // One for each memory, with all the changes of the run.
  std::vector<DreamingProposal> proposals;
  // The watermarks of the turns that the run read, set when the review ends.
  std::map<std::string, base::Time> watermarks;
};

const char* DreamingStatusToString(DreamingStatus status);
const char* LearnedMemoryCategoryToString(LearnedMemoryCategory category);
const char* LearnedMemoryTypeToString(LearnedMemoryType type);

struct DreamingResult {
  DreamingResult();
  explicit DreamingResult(DreamingStatus status);
  DreamingResult(const DreamingResult&) = delete;
  DreamingResult& operator=(const DreamingResult&) = delete;
  DreamingResult(DreamingResult&&);
  DreamingResult& operator=(DreamingResult&&);
  ~DreamingResult();

  DreamingStatus status = DreamingStatus::kCompleted;
  // The user turns that the run read, and the turns that the gate kept.
  size_t turns_read = 0;
  size_t turns_kept = 0;
  size_t memories_added = 0;
  // Same, replace and merge.
  size_t memories_updated = 0;
  // Each step with its inputs, outputs and latency, when
  // DreamingConfig::record_trace is true. The eval harness reads it.
  base::ListValue trace;
  // When DreamingConfig::review_changes is true: what the run learned, for the
  // user to review. Null when the run was canceled.
  std::optional<DreamingReview> review;
};

struct DreamingConfig {
  // A decision model answer is certain when its probability is at least
  // |certain_threshold|, and |certain_margin| above the next answer.
  double certain_threshold = 0.8;
  double certain_margin = 0.2;
  // The gate keeps a turn when "yes" is at least |gate_threshold|. It removes
  // only turns that are clearly not about the user, so it is not strict.
  double gate_threshold = 0.2;
  // A sentence needs "yes" of at least |fact_threshold| for the fact question,
  // and each unsafe answer below its limit. A wrong "drop" costs one memory, a
  // wrong "keep" can cost privacy, so sensitive data has the lowest limit.
  double fact_threshold = 0.6;
  double max_sensitive = 0.3;
  double max_instruction = 0.3;
  double max_short_lived = 0.5;
  // The work time of a run. It starts after the wait for the index.
  base::TimeDelta time_limit = base::Seconds(30);
  // A run starts when the index of the learned memories is current. A run that
  // waits longer fails, and the manager tries again later.
  base::TimeDelta index_wait_limit = base::Minutes(5);
  // LLM limits for each run. Rewrites and merges count as writing.
  size_t max_writing_requests = 50;
  size_t max_relation_requests = 20;
  // The relation question compares a fact with up to |max_neighbors| old
  // memories that are at least |min_neighbor_similarity| close to it.
  size_t max_neighbors = 5;
  float min_neighbor_similarity = 0.5f;
  // Records each step in DreamingResult::trace.
  bool record_trace = false;
  // Stores nothing and moves no watermark: the run gives its changes in
  // DreamingResult::review, for the user to review.
  bool review_changes = false;
};

// One Dreaming run. UserMemoryManager makes it, and deletes it after |done|
// runs. The run waits until the index of the learned memories is current.
// Then, for each new user turn of the stored chats, the run:
//  1. asks the gate question
//  2. splits the turn into sentences
//  3. asks the fact, safety, category and type questions
//  4. rewrites the kept sentences with the LLM (mechanical guards only: the
//  user
//     reviews the memories)
//  5. finds the closest old memories: it searches the index, and compares
//     with the memories that it wrote itself (the index does not have them
//     yet)
//  6. asks the relation question, and the LLM when it is not certain
//  7. merges with the LLM (the same guards)
//  8. stores the memory, and moves the watermark of the chat
// With DreamingConfig::review_changes, step 8 changes only the run's own copy
// of the memories, and the run gives the changes for review when it ends.
class DreamingRun {
 public:
  using DoneCallback = base::OnceCallback<void(DreamingResult)>;

  // Turns with more characters are usually pasted text, so the run skips them.
  static constexpr size_t kMaxTurnLength = 2000;

  // |data_source|, |decision_client|, |llm_engine| and |embedder| must outlive
  // the run. |llm_engine| is the engine of the local BYOM model. It can be
  // null: then the run stores the user's sentences, and does not add a fact
  // whose relation is not certain. When |search| goes away, the run fails.
  DreamingRun(LearnedMemoryDataSource& data_source,
              base::WeakPtr<LearnedMemorySearch> search,
              MemoryDecisionClient& decision_client,
              EngineConsumer* llm_engine,
              passage_embeddings::Embedder& embedder,
              DreamingConfig config,
              DoneCallback done);
  DreamingRun(const DreamingRun&) = delete;
  DreamingRun& operator=(const DreamingRun&) = delete;
  ~DreamingRun();

  void Start();

  // Stops the run with kCanceled. The run does no more storage calls.
  void Cancel();

  // Returns the typed text of |turn| when Dreaming can learn from it. Only
  // normal user questions qualify. For an edited turn, this is the text of
  // the last edit.
  static std::optional<std::string> GetLearnableText(
      const mojom::ConversationTurn& turn);

 private:
  struct UserTurn {
    std::string conversation_uuid;
    std::string entry_uuid;
    std::string text;
    base::Time date;
  };

  // A sentence of the current turn that passed the decisions.
  struct Candidate {
    std::string text;
    LearnedMemoryCategory category = LearnedMemoryCategory::kPersonalFact;
    LearnedMemoryType type = LearnedMemoryType::kLongTerm;
    MemorySourceLink link;
  };

  // A fact of the current turn, ready for the relation step.
  struct Fact {
    Fact();
    Fact(Fact&&);
    Fact& operator=(Fact&&);
    ~Fact();

    std::string text;
    std::vector<float> vector;
    LearnedMemoryCategory category = LearnedMemoryCategory::kPersonalFact;
    LearnedMemoryType type = LearnedMemoryType::kLongTerm;
    std::vector<MemorySourceLink> links;
  };

  using EmbeddingsCallback =
      base::OnceCallback<void(std::vector<std::vector<float>>)>;
  // The text of the LLM answer, or std::nullopt when the request fails.
  using LlmCallback = base::OnceCallback<void(std::optional<std::string>)>;

  // Loading.
  void OnIndexCurrent();
  void OnWatermarks(std::map<std::string, base::Time> watermarks);
  void OnMemories(std::vector<LearnedMemory> memories);
  void OnConversations(std::vector<mojom::ConversationPtr> conversations);
  void ReadNextConversation();
  void OnConversationData(std::string conversation_uuid,
                          base::Time watermark,
                          mojom::ConversationArchivePtr archive);

  // Steps 1 to 4, for each turn.
  void ProcessNextTurn();
  void OnGate(std::optional<AnswerProbabilities<bool>> gate);
  void OnSentenceDecisions(
      std::vector<size_t> sentence_indexes,
      std::vector<std::string> sentences,
      std::optional<std::vector<SentenceDecisions>> decisions);
  void OnRewrite(std::optional<std::string> answer);
  void OnFactEmbeddings(std::vector<std::vector<float>> vectors);
  void FinishTurn();
  void OnWatermarkSet(bool success);

  // Steps 5 to 8, for each fact of the turn.
  void ProcessNextFact();
  void OnNeighborMatches(std::vector<LearnedMemoryMatch> matches);
  void OnRelations(
      std::optional<std::vector<AnswerProbabilities<RelationAnswer>>>
          relations);
  void EvaluateNextRelation();
  void OnLlmRelation(std::optional<std::string> answer);
  // |certain| is true when the decision model was certain. A replace or a
  // merge changes information, so it needs a certain answer. The LLM fallback
  // can only say "same" or "different".
  void ApplyRelation(RelationAnswer relation, bool certain);
  void OnMerged(std::optional<std::string> answer);
  void OnMergedEmbedding(std::string merged,
                         std::vector<std::vector<float>> vectors);
  void AddNewMemory();
  // |embedding| is the embedding of a new text. It is null when the text did
  // not change.
  void Store(LearnedMemory memory,
             bool is_new,
             std::optional<std::vector<float>> embedding);
  void OnStored(LearnedMemory memory,
                bool is_new,
                std::optional<std::vector<float>> embedding,
                bool success);
  void NextFact();

  // |purpose| names the call in the trace.
  void AskLlm(std::string_view purpose,
              MemoryLlmRequest request,
              LlmCallback callback);
  void OnLlmAnswer(std::string purpose,
                   std::string user_message,
                   LlmCallback callback,
                   EngineConsumer::GenerationResult result);
  void Embed(std::string_view purpose,
             std::vector<std::string> passages,
             EmbeddingsCallback callback);
  void OnEmbedded(std::string purpose,
                  EmbeddingsCallback callback,
                  std::vector<std::string> passages,
                  std::vector<passage_embeddings::Embedding> embeddings,
                  uint64_t job_id,
                  passage_embeddings::ComputeEmbeddingsStatus status);

  void Finish(DreamingStatus status);
  // The changes of the turns that the run finished, against the memories at
  // the start of the run.
  DreamingReview MakeReview() const;

  // Adds a step to the trace when the config asks for it. The step gets the
  // time since the start of the run, and the current turn.
  bool tracing() const { return config_.record_trace; }
  void Trace(std::string_view step, base::DictValue data);
  void StartCall() { call_start_ = base::TimeTicks::Now(); }
  int64_t CallMs() const {
    return (base::TimeTicks::Now() - call_start_).InMilliseconds();
  }

  template <typename Answer>
  std::optional<Answer> Certain(
      const AnswerProbabilities<Answer>& probabilities) const {
    return GetCertainAnswer(probabilities, config_.certain_threshold,
                            config_.certain_margin);
  }

  const UserTurn& turn() const { return turns_.front(); }
  const Fact& fact() const { return facts_[next_fact_]; }
  LearnedMemory& neighbor() { return memories_[neighbors_[next_relation_]]; }
  bool CanWrite() const;

  const raw_ref<LearnedMemoryDataSource> data_source_;
  base::WeakPtr<LearnedMemorySearch> search_;
  const raw_ref<MemoryDecisionClient> decision_client_;
  const raw_ptr<EngineConsumer> llm_engine_;
  const raw_ref<passage_embeddings::Embedder> embedder_;
  const DreamingConfig config_;
  DoneCallback done_;

  std::map<std::string, base::Time> watermarks_;
  std::vector<LearnedMemory> memories_;
  // With review_changes: the memories at the start of the run and at the start
  // of the current turn, and the watermarks of the finished turns. A turn that
  // did not finish is not in the review: the next run reads it again.
  std::vector<LearnedMemory> stored_memories_;
  std::optional<std::vector<LearnedMemory>> turn_start_memories_;
  std::map<std::string, base::Time> review_watermarks_;
  // The embeddings of the texts that this run wrote, by memory uuid. The index
  // gets them only after its next sync, so the neighbor search uses these.
  std::map<std::string, std::vector<float>> run_embeddings_;
  base::circular_deque<std::string> conversations_to_read_;
  base::circular_deque<UserTurn> turns_;

  // The current turn.
  std::vector<Candidate> candidates_;
  std::vector<Fact> facts_;
  size_t next_fact_ = 0;
  // The current fact: indexes into |memories_|, and their relations.
  std::vector<size_t> neighbors_;
  std::vector<AnswerProbabilities<RelationAnswer>> relations_;
  size_t next_relation_ = 0;

  size_t writing_requests_ = 0;
  size_t relation_requests_ = 0;
  std::optional<passage_embeddings::Embedder::Job> embed_job_;
  base::TimeTicks run_start_;
  base::TimeTicks call_start_;
  DreamingResult result_;
  base::OneShotTimer index_wait_timer_;
  base::OneShotTimer time_limit_timer_;

  base::WeakPtrFactory<DreamingRun> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_DREAMING_RUN_H_
