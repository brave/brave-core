// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/user_memory_manager.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/learned_memory_eval.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

// static
DreamingConfig UserMemoryManager::GetDreamingConfigFromFeatures() {
  DreamingConfig config;
  config.certain_threshold = features::kLearnedMemoryCertainThreshold.Get();
  config.certain_margin = features::kLearnedMemoryCertainMargin.Get();
  config.gate_threshold = features::kLearnedMemoryGateThreshold.Get();
  config.fact_threshold = features::kLearnedMemoryFactThreshold.Get();
  config.time_limit = features::kLearnedMemoryRunTimeLimit.Get();
  config.max_writing_requests =
      features::kLearnedMemoryMaxRewriteRequests.Get();
  config.max_relation_requests =
      features::kLearnedMemoryMaxRelationRequests.Get();
  config.record_trace = LearnedMemoryEval::IsEnabled();
  return config;
}

UserMemoryManager::UserMemoryManager(
    std::unique_ptr<MemoryDecisionClient> decision_client,
    LlmEngineFactory llm_engine_factory,
    passage_embeddings::Embedder* embedder,
    PrefService* prefs,
    LearnedMemoryDataSource* data_source,
    DreamingConfig config,
    TurnMemoryConfig turn_config)
    : decision_client_(std::move(decision_client)),
      llm_engine_factory_(std::move(llm_engine_factory)),
      embedder_(embedder),
      prefs_(prefs),
      config_(config),
      data_source_(data_source),
      turn_config_(turn_config) {
  CHECK(decision_client_);
  CHECK(llm_engine_factory_);
  CHECK(embedder_);
  CHECK(prefs_);
  CHECK(data_source_);
}

UserMemoryManager::~UserMemoryManager() = default;

void UserMemoryManager::OnStorageReady() {
  if (!data_source_->IsStorageReady()) {
    return;
  }
  if (LearnedMemoryEval::IsEnabled()) {
    if (!eval_) {
      eval_ = LearnedMemoryEval::CreateFromCommandLine();
      eval_->Start(*data_source_, config_,
                   base::BindOnce(&UserMemoryManager::LearnFromChats,
                                  base::Unretained(this)));
    }
    return;
  }
  ScheduleNextDailyDreaming();
}

void UserMemoryManager::OnAllConversationsDeleted() {
  // The chats and the memories are gone, so the run has nothing to continue.
  // Cancel it first: its callback sees the end of the run.
  if (dreaming_run_) {
    dreaming_run_->Cancel();
  }
  if (!data_source_->IsStorageReady()) {
    // Storage is off. OnStorageReady() starts everything again.
    dreaming_timer_.Stop();
    eval_.reset();
    return;
  }
  // Storage is on, and a new chat starts a new history. Dreaming goes on with
  // its schedule.
  if (!LearnedMemoryEval::IsEnabled() && !dreaming_timer_.IsRunning()) {
    ScheduleNextDailyDreaming();
  }
}

void UserMemoryManager::LearnFromChats(DreamingCallback callback) {
  StartRun(std::move(callback), config_);
}

void UserMemoryManager::DreamNow(DreamingCallback callback) {
  if (!prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled)) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kUnavailable));
    return;
  }
  DreamingConfig config = config_;
  config.time_limit = std::max(config.time_limit, kDreamNowTimeLimit);
  StartRun(std::move(callback), config);
}

void UserMemoryManager::GetLearnedMemories(LearnedMemoriesCallback callback) {
  data_source_->GetLearnedMemories(std::move(callback));
}

void UserMemoryManager::DeleteLearnedMemory(
    const std::string& uuid,
    base::OnceCallback<void(bool)> callback) {
  data_source_->DeleteLearnedMemory(uuid, std::move(callback));
}

void UserMemoryManager::GetMemoriesForTurn(
    std::vector<std::string> user_messages,
    TurnMemoriesCallback callback) {
  if (!data_source_->IsStorageReady() ||
      !prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled) ||
      user_messages.empty()) {
    std::move(callback).Run({});
    return;
  }
  const uint64_t id = next_turn_lookup_id_++;
  auto lookup = std::make_unique<TurnMemoryLookup>(
      *data_source_, *decision_client_, *embedder_, turn_config_,
      std::move(user_messages),
      base::BindOnce(&UserMemoryManager::OnTurnLookupDone,
                     weak_ptr_factory_.GetWeakPtr(), id, std::move(callback)));
  TurnMemoryLookup* lookup_ptr = lookup.get();
  turn_lookups_[id] = std::move(lookup);
  lookup_ptr->Start();
}

void UserMemoryManager::OnTurnLookupDone(
    uint64_t lookup_id,
    TurnMemoriesCallback callback,
    EngineConsumer::LearnedMemories memories) {
  // The lookup calls this as its last step, and it is still on the stack.
  // Delete it later.
  auto it = turn_lookups_.find(lookup_id);
  if (it != turn_lookups_.end()) {
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(it->second));
    turn_lookups_.erase(it);
  }
  std::move(callback).Run(std::move(memories));
}

void UserMemoryManager::StartRun(DreamingCallback callback,
                                 const DreamingConfig& config) {
  if (dreaming_run_) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kBusy));
    return;
  }
  if (!data_source_->IsStorageReady()) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kUnavailable));
    return;
  }
  dreaming_timer_.Stop();
  dreaming_callback_ = std::move(callback);
  run_llm_engine_ = llm_engine_factory_.Run();
  VLOG(1) << "Dreaming starts, local LLM: " << (run_llm_engine_ ? "yes" : "no");
  dreaming_run_ = std::make_unique<DreamingRun>(
      *data_source_, *decision_client_, run_llm_engine_.get(), *embedder_,
      config,
      base::BindOnce(&UserMemoryManager::OnDreamingDone,
                     base::Unretained(this)));
  dreaming_run_->Start();
}

void UserMemoryManager::ScheduleDreaming(base::TimeDelta delay) {
  if (!data_source_->IsStorageReady()) {
    return;
  }
  VLOG(1) << "Next Dreaming run in " << delay;
  dreaming_timer_.Start(FROM_HERE, delay,
                        base::BindOnce(&UserMemoryManager::OnDreamingTimer,
                                       base::Unretained(this)));
}

void UserMemoryManager::ScheduleNextDailyDreaming() {
  const base::Time last = prefs_->GetTime(prefs::kBraveAIChatLastDreamingTime);
  base::TimeDelta delay = kFirstRunDelay;
  if (!last.is_null()) {
    delay = std::clamp(last + kDreamingInterval - base::Time::Now(),
                       kFirstRunDelay, kDreamingInterval);
  }
  ScheduleDreaming(delay);
}

void UserMemoryManager::OnDreamingTimer() {
  if (!prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled)) {
    // Memory is off. Check again later.
    ScheduleDreaming(kRetryDelay);
    return;
  }
  // The result goes to OnDreamingDone(), which schedules the next run.
  LearnFromChats(base::DoNothing());
}

void UserMemoryManager::OnDreamingDone(DreamingResult result) {
  VLOG(1) << "Dreaming " << DreamingStatusToString(result.status)
          << ": turns read " << result.turns_read << ", kept "
          << result.turns_kept << ", memories added " << result.memories_added
          << ", updated " << result.memories_updated;
  // The run calls this as its last step, and it is still on the stack. Delete
  // it later, and the engine after it.
  auto task_runner = base::SequencedTaskRunner::GetCurrentDefault();
  task_runner->DeleteSoon(FROM_HERE, std::move(dreaming_run_));
  task_runner->DeleteSoon(FROM_HERE, std::move(run_llm_engine_));

  // In eval mode, the eval reports the result, and no timer runs.
  const bool schedule = !LearnedMemoryEval::IsEnabled();
  switch (schedule ? result.status : DreamingStatus::kCanceled) {
    case DreamingStatus::kCompleted:
    case DreamingStatus::kTimedOut:
      // A run that timed out continues from the watermark in the next run.
      prefs_->SetTime(prefs::kBraveAIChatLastDreamingTime, base::Time::Now());
      ScheduleNextDailyDreaming();
      break;
    case DreamingStatus::kFailed:
      ScheduleDreaming(kRetryDelay);
      break;
    case DreamingStatus::kCanceled:
    case DreamingStatus::kBusy:
    case DreamingStatus::kUnavailable:
      // Storage is gone or the chats were deleted. OnStorageReady() and
      // OnAllConversationsDeleted() schedule again.
      break;
  }
  // Reset first, so the callback can start a new run.
  DreamingCallback callback = std::move(dreaming_callback_);
  std::move(callback).Run(std::move(result));
}

}  // namespace ai_chat
