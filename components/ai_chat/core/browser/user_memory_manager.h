// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/dreaming_run.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

class PrefService;

namespace ai_chat {

class AIChatDatabase;
class LearnedMemoryEval;

// Learns memories from past chats (Dreaming) and finds the memories that fit a
// chat turn. AIChatService owns one instance. A timer starts Dreaming one time
// each day while the database is available.
class UserMemoryManager {
 public:
  using DreamingCallback = base::OnceCallback<void(DreamingResult)>;
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

  // Reads the Dreaming settings from the learned memory feature params. In eval
  // mode (see learned_memory_eval.h), the run records a trace.
  static DreamingConfig GetDreamingConfigFromFeatures();

  // |embedder| and |prefs| must outlive the manager.
  UserMemoryManager(std::unique_ptr<MemoryDecisionClient> decision_client,
                    LlmEngineFactory llm_engine_factory,
                    passage_embeddings::Embedder* embedder,
                    PrefService* prefs,
                    DreamingConfig config);
  UserMemoryManager(const UserMemoryManager&) = delete;
  UserMemoryManager& operator=(const UserMemoryManager&) = delete;
  ~UserMemoryManager();

  // AIChatService calls these when the chat database comes and goes. |db| must
  // stay valid until OnDatabaseUnavailable(). The timer runs only while the
  // database is available. In eval mode, the manager runs the eval one time
  // instead of the timer.
  void OnDatabaseAvailable(base::SequenceBound<AIChatDatabase>* db);
  // Stops the timer and the current run.
  void OnDatabaseUnavailable();

  bool is_database_available() const { return db_ != nullptr; }
  bool is_dreaming() const { return !!dreaming_run_; }
  bool is_dreaming_scheduled() const { return dreaming_timer_.IsRunning(); }

  // Starts a Dreaming run, and runs |callback| when the run ends. Only one run
  // can be active. When the manager cannot start a run, |callback| runs
  // with kBusy or kUnavailable.
  void LearnFromChats(DreamingCallback callback);

 private:
  void ScheduleDreaming(base::TimeDelta delay);
  void ScheduleNextDailyDreaming();
  void OnDreamingTimer();
  void OnDreamingDone(DreamingResult result);

  std::unique_ptr<MemoryDecisionClient> decision_client_;
  LlmEngineFactory llm_engine_factory_;
  const raw_ptr<passage_embeddings::Embedder> embedder_;
  const raw_ptr<PrefService> prefs_;
  const DreamingConfig config_;
  raw_ptr<base::SequenceBound<AIChatDatabase>> db_ = nullptr;

  base::OneShotTimer dreaming_timer_;
  // The LLM engine of the current run. It must outlive |dreaming_run_|.
  std::unique_ptr<EngineConsumer> run_llm_engine_;
  std::unique_ptr<DreamingRun> dreaming_run_;
  DreamingCallback dreaming_callback_;
  std::unique_ptr<LearnedMemoryEval> eval_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_USER_MEMORY_MANAGER_H_
