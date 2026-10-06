// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_EVAL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_EVAL_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/dreaming_run.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

class AIChatDatabase;

// Eval mode for learned memory, for the harness in
// brave/tools/learned_memory_eval. Start the browser with both switches:
//   --learned-memory-eval-chats=<chat set JSON>
//   --learned-memory-eval-output=<report JSON>
// When the database comes, UserMemoryManager imports the chats, runs Dreaming
// one time with the trace on, and writes the report. The timer does not run.
inline constexpr char kLearnedMemoryEvalChatsSwitch[] =
    "learned-memory-eval-chats";
inline constexpr char kLearnedMemoryEvalOutputSwitch[] =
    "learned-memory-eval-output";

// A chat of the chat set, ready for the database.
struct EvalChat {
  EvalChat();
  EvalChat(EvalChat&&);
  EvalChat& operator=(EvalChat&&);
  ~EvalChat();

  mojom::ConversationPtr conversation;
  // The user and assistant turns, from the oldest.
  std::vector<mojom::ConversationTurnPtr> entries;
};

class LearnedMemoryEval {
 public:
  // Starts a Dreaming run and gives its result.
  using RunDreamingCallback = base::OnceCallback<void(
      base::OnceCallback<void(DreamingResult)> on_done)>;

  // True when both switches are set.
  static bool IsEnabled();
  // Returns null when IsEnabled() is false.
  static std::unique_ptr<LearnedMemoryEval> CreateFromCommandLine();

  // Parses a chat set. Format:
  //   {"name": "...", "chats": [{"title": "...", "days_ago": 30,
  //     "turns": [{"user": "...", "assistant": "..."}]}]}
  // The conversation uuids are "eval-<chat>", and the entry uuids are
  // "eval-<chat>-<turn>-user" and "eval-<chat>-<turn>-assistant". Other keys,
  // for example the expectations of the harness, are ignored.
  static std::optional<std::vector<EvalChat>> ParseChatSet(
      std::string_view json,
      base::Time now);

  LearnedMemoryEval(base::FilePath chats_path, base::FilePath output_path);
  LearnedMemoryEval(const LearnedMemoryEval&) = delete;
  LearnedMemoryEval& operator=(const LearnedMemoryEval&) = delete;
  ~LearnedMemoryEval();

  // |db| must outlive this object.
  void Start(base::SequenceBound<AIChatDatabase>& db,
             DreamingConfig config,
             RunDreamingCallback run_dreaming);

 private:
  void OnChatsRead(std::optional<std::string> json);
  void OnImported(std::vector<mojom::ConversationPtr> conversations);
  void OnDreamingDone(DreamingResult result);
  void OnMemories(DreamingResult result, std::vector<LearnedMemory> memories);
  void Fail(std::string_view error);

  const base::FilePath chats_path_;
  const base::FilePath output_path_;
  raw_ptr<base::SequenceBound<AIChatDatabase>> db_ = nullptr;
  DreamingConfig config_;
  RunDreamingCallback run_dreaming_;
  size_t chat_count_ = 0;

  base::WeakPtrFactory<LearnedMemoryEval> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_LEARNED_MEMORY_EVAL_H_
