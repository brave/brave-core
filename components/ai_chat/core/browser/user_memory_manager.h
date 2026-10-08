// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/dreaming_run.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/learned_memory_data_source.h"
#include "brave/components/ai_chat/core/browser/learned_memory_search.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/ai_chat/core/browser/turn_memory_lookup.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

class PrefService;

namespace ai_chat {

class LearnedMemoryEval;

// A change of the review that the user keeps, and its text when the user
// edited it.
struct DreamingReviewDecision {
  std::string uuid;
  std::optional<std::string> text;
};

// Learns memories from past chats (Dreaming) and finds the memories that fit a
// chat turn. AIChatService owns one instance, and registers it as an observer.
// A timer starts Dreaming one time each day while chat history storage is on.
// With DreamingConfig::review_changes, a run stores nothing: its changes wait
// in memory for the user's review, and no run starts before the review ends.
// All chat data comes from the LearnedMemoryDataSource, which is the
// AIChatService, so the manager holds no database.
class UserMemoryManager : public AIChatService::Observer {
 public:
  using DreamingCallback = base::OnceCallback<void(DreamingResult)>;
  using LearnedMemoriesCallback =
      base::OnceCallback<void(std::vector<LearnedMemory>)>;
  using TurnMemoriesCallback =
      base::OnceCallback<void(EngineConsumer::LearnedMemories)>;
  // Makes the engine of the local LLM for one run. Gives null when the LLM is
  // not available: then Dreaming stores the user's sentences.
  using LlmEngineFactory =
      base::RepeatingCallback<std::unique_ptr<EngineConsumer>()>;

  static constexpr base::TimeDelta kDreamingInterval = base::Days(1);
  // The first run waits after the database comes, so that it does not slow
  // down the browser start.
  static constexpr base::TimeDelta kFirstRunDelay = base::Minutes(1);
  // After a failed run, for example when Ollama is not running.
  static constexpr base::TimeDelta kRetryDelay = base::Hours(1);
  // The time limit of a run that the user starts ("Dream now"). The user waits
  // for it, so it is longer than the limit of the daily run, unless the feature
  // param is longer.
  static constexpr base::TimeDelta kDreamNowTimeLimit = base::Minutes(5);

  // Reads the Dreaming settings from the learned memory feature params. In eval
  // mode (see learned_memory_eval.h), the run records a trace.
  static DreamingConfig GetDreamingConfigFromFeatures();

  // |embedder|, |prefs| and |data_source| must outlive the manager.
  UserMemoryManager(
      std::unique_ptr<MemoryDecisionClient> decision_client,
      LlmEngineFactory llm_engine_factory,
      passage_embeddings::Embedder* embedder,
      PrefService* prefs,
      LearnedMemoryDataSource* data_source,
      DreamingConfig config,
      TurnMemoryConfig turn_config = TurnMemoryConfig::FromFeatures());
  UserMemoryManager(const UserMemoryManager&) = delete;
  UserMemoryManager& operator=(const UserMemoryManager&) = delete;
  ~UserMemoryManager() override;

  // The search of the embeddings service. Without it, Dreaming does not run,
  // and a chat turn gets only the permanent memories. The embeddings service
  // sets it after it starts.
  void SetLearnedMemorySearch(base::WeakPtr<LearnedMemorySearch> search);

  // AIChatService::Observer:
  // Chat history storage became ready. The timer runs only while storage is
  // ready. In eval mode, the manager runs the eval one time instead of the
  // timer. AIChatService also calls this when it makes the manager after
  // storage became ready.
  void OnStorageReady() override;
  // The chats and the learned memories are gone: either all chats were deleted
  // or storage was turned off. Stops the current run, and the timer if storage
  // is off.
  void OnAllConversationsDeleted() override;
  // The review is dropped when a change in it came from the deleted chat or
  // turn: the change would bring the deleted source back. The watermarks did
  // not move, so the next run learns again from the chats that are left.
  void OnConversationDeleted(const std::string& conversation_uuid) override;
  void OnConversationEntryRemoved(const std::string& conversation_uuid,
                                  const std::string& entry_uuid) override;

  bool is_storage_ready() const { return data_source_->IsStorageReady(); }
  bool has_learned_memory_search() const { return !!search_; }
  bool is_dreaming() const { return !!dreaming_run_; }
  bool is_dreaming_scheduled() const { return dreaming_timer_.IsRunning(); }

  // Starts a Dreaming run, and runs |callback| when the run ends. Only one run
  // can be active. When the manager cannot start a run (no storage or no
  // search), |callback| runs with kBusy or kUnavailable, and with
  // kReviewPending while a review waits.
  void LearnFromChats(DreamingCallback callback);

  // Same as LearnFromChats(), for the "Dream now" button. The run has a longer
  // time limit. When the memory setting is off, |callback| runs with
  // kUnavailable.
  void DreamNow(DreamingCallback callback);

  // Gives all learned memories, oldest first. |callback| gets an empty list
  // when storage is not ready.
  void GetLearnedMemories(LearnedMemoriesCallback callback);

  // Deletes the memory for good. Nothing remembers it, so Dreaming can learn
  // the same fact again from another chat. Its change in the review goes too.
  // |callback| gets false when storage is not ready or the delete failed.
  void DeleteLearnedMemory(const std::string& uuid,
                           base::OnceCallback<void(bool)> callback);

  // Deletes all learned memories, and cancels the current run first: the run
  // would write the memories that it loaded again. The watermarks stay, so
  // Dreaming learns only from new turns. A review that waits is discarded, and
  // its watermarks move. |callback| gets false when storage is not ready or
  // the delete failed.
  void DeleteAllLearnedMemories(base::OnceCallback<void(bool)> callback);

  // The changes of the last run that wait for the user's review, or null.
  const DreamingReview* pending_review() const {
    return pending_review_ ? &*pending_review_ : nullptr;
  }

  // Ends the review: stores the changes in |kept|, with the text that the user
  // edited, discards the others, and moves the watermarks of the run. A change
  // of a memory that changed after the run is not stored. |callback| gets
  // false when no review waits, an edited text is empty or too long (then the
  // review stays), or a write failed.
  void ApplyDreamingReview(std::vector<DreamingReviewDecision> kept,
                           base::OnceCallback<void(bool)> callback);

  // Chat time. Finds the learned memories for a chat turn, and runs |callback|
  // with them. |user_messages| are the last user messages, the newest first.
  // |callback| runs at once with no memories when the memory setting is off or
  // storage is not ready. It runs later after the lookup, or after the
  // time out (see TurnMemoryConfig) with only the permanent memories. A lookup
  // that is still active when the manager is deleted does not run |callback|.
  void GetMemoriesForTurn(std::vector<std::string> user_messages,
                          TurnMemoriesCallback callback);

  base::WeakPtr<UserMemoryManager> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  void StartRun(DreamingCallback callback, const DreamingConfig& config);
  // The eval starts one time, when storage is ready and the search is set.
  void MaybeStartEval();
  void OnTurnLookupDone(uint64_t lookup_id,
                        TurnMemoriesCallback callback,
                        EngineConsumer::LearnedMemories memories);
  void ScheduleDreaming(base::TimeDelta delay);
  void ScheduleNextDailyDreaming();
  void OnDreamingTimer();
  void OnDreamingDone(DreamingResult result);
  void OnStampsForReview(std::vector<DreamingProposal> kept,
                         std::map<std::string, base::Time> watermarks,
                         base::OnceCallback<void(bool)> callback,
                         std::vector<LearnedMemoryStamp> stamps);
  void OnReviewApplied(base::OnceCallback<void(bool)> callback,
                       std::vector<bool> results);
  // Moves the watermarks, and ignores the results.
  void SetWatermarks(const std::map<std::string, base::Time>& watermarks);
  // Drops the review when a change in it has a source that |matches|.
  void DropReviewWithSource(
      base::FunctionRef<bool(const MemorySourceLink&)> matches);

  std::unique_ptr<MemoryDecisionClient> decision_client_;
  LlmEngineFactory llm_engine_factory_;
  const raw_ptr<passage_embeddings::Embedder> embedder_;
  const raw_ptr<PrefService> prefs_;
  const DreamingConfig config_;
  const raw_ptr<LearnedMemoryDataSource> data_source_;
  base::WeakPtr<LearnedMemorySearch> search_;
  const TurnMemoryConfig turn_config_;

  base::OneShotTimer dreaming_timer_;
  // The LLM engine of the current run. It must outlive |dreaming_run_|.
  std::unique_ptr<EngineConsumer> run_llm_engine_;
  std::unique_ptr<DreamingRun> dreaming_run_;
  DreamingCallback dreaming_callback_;
  std::optional<DreamingReview> pending_review_;
  // The writes of a review are on their way. No run starts before they land.
  bool applying_review_ = false;
  std::unique_ptr<LearnedMemoryEval> eval_;
  // The active lookups for chat turns.
  std::map<uint64_t, std::unique_ptr<TurnMemoryLookup>> turn_lookups_;
  uint64_t next_turn_lookup_id_ = 1;

  base::WeakPtrFactory<UserMemoryManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_
