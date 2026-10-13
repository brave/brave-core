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
#include <string_view>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/functional/callback.h"
#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
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

// What started a Dreaming run.
enum class DreamingTrigger { kTimer, kDreamNow, kEval };

// A Dreaming run for brave://leo-memory-internals: when and why it ran, how it
// ended, and its steps. Kept in memory only.
struct DreamingRunRecord {
  DreamingRunRecord();
  DreamingRunRecord(const DreamingRunRecord&) = delete;
  DreamingRunRecord& operator=(const DreamingRunRecord&) = delete;
  DreamingRunRecord(DreamingRunRecord&&);
  DreamingRunRecord& operator=(DreamingRunRecord&&);
  ~DreamingRunRecord();

  uint64_t id = 0;
  DreamingTrigger trigger = DreamingTrigger::kTimer;
  base::Time start_time;
  // Null while the run is active.
  std::optional<base::Time> end_time;
  std::optional<DreamingStatus> status;
  // The steps (DreamingResult::trace): the requests to the decision model and
  // the LLM, their answers, and the decisions that the run made from them.
  base::ListValue trace;
};

// The learned memory lookup of one chat turn, for brave://leo-memory-internals.
// Kept in memory only.
struct TurnLookupRecord {
  TurnLookupRecord();
  TurnLookupRecord(const TurnLookupRecord&) = delete;
  TurnLookupRecord& operator=(const TurnLookupRecord&) = delete;
  TurnLookupRecord(TurnLookupRecord&&);
  TurnLookupRecord& operator=(TurnLookupRecord&&);
  ~TurnLookupRecord();

  uint64_t id = 0;
  std::string conversation_uuid;
  base::Time start_time;
  // Null while the lookup is active.
  std::optional<base::Time> end_time;
  // The steps (TurnMemoryLookup::trace()).
  base::ListValue trace;
  // The request to the decision model as sent and its answer, with
  // "arrived_ms" since the start. An answer can come after the lookup ended.
  base::ListValue decision_requests;
};

// A change of the review that the user keeps, and its text when the user
// edited it.
struct DreamingReviewDecision {
  std::string uuid;
  std::optional<std::string> text;
};

// A change of a learned memory that is not a step of a Dreaming run: a choice
// of the user, the clean-up of the old memories, or a save of the memory tool.
// For brave://leo-memory-internals. Kept in memory only.
struct MemoryEventRecord {
  MemoryEventRecord();
  MemoryEventRecord(const MemoryEventRecord&) = delete;
  MemoryEventRecord& operator=(const MemoryEventRecord&) = delete;
  MemoryEventRecord(MemoryEventRecord&&);
  MemoryEventRecord& operator=(MemoryEventRecord&&);
  ~MemoryEventRecord();

  uint64_t id = 0;
  base::Time time;
  // "keep_forever", "release_keep_forever", "edited", "deleted",
  // "deleted_all", "expired", "tool_saved" or "tool_duplicate".
  std::string action;
  // The learned memory. Empty for a memory of the pref list and for
  // "deleted_all".
  std::string uuid;
  std::string text;
  base::DictValue details;
};

// Why an edit of a learned memory failed.
enum class LearnedMemoryEditError {
  // The text is empty or too long.
  kInvalidLength,
  // The memory is not stored, storage is not ready, or a review is being
  // written.
  kNotFound,
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
  // How often the manager deletes the learned memories that expired (see
  // GetLearnedMemoryExpiryTime()). The first clean-up waits kFirstRunDelay.
  static constexpr base::TimeDelta kExpiryInterval = base::Days(1);
  // The date of the last use of a memory moves at most once in this time.
  static constexpr base::TimeDelta kUseGranularity = base::Days(1);
  // After a failed run, for example when Ollama is not running.
  static constexpr base::TimeDelta kRetryDelay = base::Hours(1);
  // The time limit of a run that the user starts ("Learn from chats"). The user
  // waits for it, so it is longer than the limit of the daily run, unless the
  // feature param is longer.
  static constexpr base::TimeDelta kDreamNowTimeLimit = base::Minutes(5);

  // The finished runs that the manager keeps for brave://leo-memory-internals.
  static constexpr size_t kMaxDreamingRunRecords = 10;
  // The chat turn lookups that the manager keeps for the same page.
  static constexpr size_t kMaxTurnLookupRecords = 20;
  // The memory events that the manager keeps for the same page.
  static constexpr size_t kMaxMemoryEventRecords = 100;

  // Reads the Dreaming settings from the learned memory feature params. Each
  // run records a trace.
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

  // Same as LearnFromChats(), for the "Learn from chats" button. The run has a
  // longer time limit. When the memory setting is off, |callback| runs with
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

  // Keeps the memory as it is (|permanent| true): Dreaming never replaces or
  // merges it, it never expires, and Leo sends it in every chat. With false,
  // the memory is long-term again. A change of the last run's review that
  // waits for the memory is dropped when it is kept, and the run's steps tell
  // that. |callback| gets false when the memory is not stored, a review is
  // being written, or the write failed.
  void SetLearnedMemoryPermanent(const std::string& uuid,
                                 bool permanent,
                                 base::OnceCallback<void(bool)> callback);

  // Changes the text of a learned memory to |text| (trimmed). The sources and
  // the type stay, and the old text becomes the "before" text. A change of the
  // review that waits for the memory is dropped: the user's text wins. The
  // search index follows through the text version. |callback| gets null on
  // success.
  using EditLearnedMemoryCallback =
      base::OnceCallback<void(std::optional<LearnedMemoryEditError>)>;
  void EditLearnedMemory(const std::string& uuid,
                         const std::string& text,
                         EditLearnedMemoryCallback callback);

  // Deletes all learned memories, and cancels the current run first: the run
  // would write the memories that it loaded again. The watermarks stay, so
  // Dreaming learns only from new turns. A review that waits is discarded, and
  // its watermarks move. |callback| gets false when storage is not ready or
  // the delete failed.
  void DeleteAllLearnedMemories(base::OnceCallback<void(bool)> callback);

  // The active run and the last kMaxDreamingRunRecords runs, the newest first,
  // with a copy of their traces.
  std::vector<DreamingRunRecord> GetDreamingRunRecords() const;

  // Learned memories were used: Leo sent them to a chat model, or a search tool
  // found them. Their last use moves to now, and so does their expiry date. A
  // memory that the user keeps does not expire, so it is left alone. The date
  // moves at most once in kUseGranularity for a memory, to keep the writes
  // few. |used_by| names the use in the event ("chat_turn" or
  // "memory_search_tool").
  void MarkLearnedMemoriesUsed(std::vector<std::string> uuids,
                               std::string_view used_by);

  // Deletes the learned memories that expired: not used for kShortTermMemory-
  // Lifetime (short-term) or kLongTermMemoryLifetime (long-term). The daily
  // timer calls it. A change of the review for such a memory is dropped.
  void ExpireLearnedMemories();

  // Adds an event for brave://leo-memory-internals. Also called by the memory
  // tool through AIChatService.
  void RecordMemoryEvent(std::string_view action,
                         std::string_view uuid,
                         std::string_view text,
                         base::DictValue details = base::DictValue());

  // The last kMaxMemoryEventRecords events, the newest first.
  std::vector<MemoryEventRecord> GetMemoryEventRecords() const;

  // The last kMaxTurnLookupRecords chat turn lookups, the newest first, with
  // the steps so far of the active ones.
  std::vector<TurnLookupRecord> GetTurnLookupRecords() const;

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
  // with them. |conversation_uuid| is the chat of the turn, for the record of
  // the lookup. |user_messages| are the last user messages, the newest first.
  // |callback| runs at once with no memories when the memory setting is off or
  // storage is not ready. It runs later after the lookup, or after the
  // time out (see TurnMemoryConfig) with only the permanent memories. A lookup
  // that is still active when the manager is deleted does not run |callback|.
  void GetMemoriesForTurn(const std::string& conversation_uuid,
                          std::vector<std::string> user_messages,
                          TurnMemoriesCallback callback);

  base::WeakPtr<UserMemoryManager> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  void StartRun(DreamingCallback callback,
                const DreamingConfig& config,
                DreamingTrigger trigger);
  // The eval starts one time, when storage is ready and the search is set.
  void MaybeStartEval();
  void OnTurnLookupDone(uint64_t lookup_id,
                        TurnMemoriesCallback callback,
                        EngineConsumer::LearnedMemories memories);
  void OnTurnExchange(uint64_t lookup_id,
                      MemoryDecisionClient::Exchange exchange);
  TurnLookupRecord* FindTurnRecord(uint64_t lookup_id);
  void ScheduleDreaming(base::TimeDelta delay);
  void ScheduleNextDailyDreaming();
  void OnDreamingTimer();
  void OnDreamingDone(DreamingResult result);
  void OnStampsForReview(std::vector<DreamingProposal> kept,
                         uint64_t run_id,
                         size_t discarded,
                         std::map<std::string, base::Time> watermarks,
                         base::OnceCallback<void(bool)> callback,
                         std::vector<LearnedMemoryStamp> stamps);
  void OnReviewApplied(base::OnceCallback<void(bool)> callback,
                       std::vector<bool> results);
  void OnMemoriesToMarkUsed(std::string used_by,
                            std::vector<LearnedMemory> memories);
  void OnMemoriesToExpire(std::vector<LearnedMemory> memories);
  void ScheduleExpiry(base::TimeDelta delay);
  void OnExpiryTimer();
  void OnMemoriesForEdit(const std::string& uuid,
                         std::string text,
                         EditLearnedMemoryCallback callback,
                         std::vector<LearnedMemory> memories);
  // Drops the change of the review that waits for the memory, if any, and
  // tells the run's steps why. Returns true when it dropped a change.
  bool DropPendingChange(const std::string& uuid,
                         std::string_view step,
                         base::DictValue data);
  void OnMemoriesForPermanent(const std::string& uuid,
                              bool permanent,
                              base::OnceCallback<void(bool)> callback,
                              std::vector<LearnedMemory> memories);
  // Makes the change of the review that waits fit the new type of a memory.
  void UpdateReviewForType(const LearnedMemory& memory);
  // Adds a step to the trace of a finished run. The step is made after the
  // run, for example by the user's review.
  void AppendRunStep(uint64_t run_id,
                     std::string_view step,
                     base::DictValue data);
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
  base::OneShotTimer expiry_timer_;
  // The LLM engine of the current run. It must outlive |dreaming_run_|.
  std::unique_ptr<EngineConsumer> run_llm_engine_;
  std::unique_ptr<DreamingRun> dreaming_run_;
  DreamingCallback dreaming_callback_;
  std::optional<DreamingReview> pending_review_;
  // The run that made |pending_review_|, for its record.
  uint64_t pending_review_run_id_ = 0;
  // The record of the active run, without its trace, and the finished runs.
  std::optional<DreamingRunRecord> active_run_record_;
  base::circular_deque<DreamingRunRecord> run_records_;
  uint64_t next_run_id_ = 1;
  // The writes of a review are on their way. No run starts before they land.
  bool applying_review_ = false;
  std::unique_ptr<LearnedMemoryEval> eval_;
  // The active lookups for chat turns.
  std::map<uint64_t, std::unique_ptr<TurnMemoryLookup>> turn_lookups_;
  uint64_t next_turn_lookup_id_ = 1;
  base::circular_deque<TurnLookupRecord> turn_records_;
  base::circular_deque<MemoryEventRecord> event_records_;
  uint64_t next_event_id_ = 1;

  base::WeakPtrFactory<UserMemoryManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_
