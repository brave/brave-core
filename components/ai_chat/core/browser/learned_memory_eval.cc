// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/learned_memory_eval.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/common/features.h"

namespace ai_chat {

namespace {

// The user and assistant turns of a chat are this far apart.
constexpr base::TimeDelta kTurnGap = base::Minutes(2);

mojom::ConversationTurnPtr MakeTurn(std::string uuid,
                                    mojom::CharacterType character,
                                    std::string text,
                                    base::Time date) {
  std::optional<std::vector<mojom::ConversationEntryEventPtr>> events;
  mojom::ActionType action = mojom::ActionType::QUERY;
  if (character == mojom::CharacterType::ASSISTANT) {
    action = mojom::ActionType::RESPONSE;
    events.emplace();
    events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(text)));
  }
  return mojom::ConversationTurn::New(
      std::move(uuid), std::nullopt, character, action, std::move(text),
      std::nullopt, std::nullopt, std::move(events), date, std::nullopt,
      std::nullopt, nullptr, false, std::nullopt, nullptr,
      std::vector<std::string>{});
}

base::DictValue MemoryToDict(const LearnedMemory& memory) {
  base::ListValue links;
  for (const auto& link : memory.links) {
    links.Append(base::DictValue()
                     .Set("conversation", link.conversation_uuid)
                     .Set("entry", link.entry_uuid)
                     .Set("sentence", static_cast<int>(link.sentence_index)));
  }
  return base::DictValue()
      .Set("text", memory.text)
      .Set("category", LearnedMemoryCategoryToString(memory.category))
      .Set("type", LearnedMemoryTypeToString(memory.type))
      .Set("created", base::TimeFormatAsIso8601(memory.created_date))
      .Set("updated", base::TimeFormatAsIso8601(memory.updated_date))
      .Set("links", std::move(links))
      .Set("previous", memory.previous ? base::Value(memory.previous->text)
                                       : base::Value());
}

std::optional<std::string> ReadFile(base::FilePath path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::nullopt;
  }
  return contents;
}

void WriteReport(base::FilePath path, base::DictValue report) {
  std::optional<std::string> json = base::WriteJsonWithOptions(
      report, base::JSONWriter::OPTIONS_PRETTY_PRINT);
  if (!json || !base::WriteFile(path, *json)) {
    LOG(ERROR) << "Learned memory eval: cannot write " << path;
    return;
  }
  LOG(WARNING) << "Learned memory eval: wrote " << path;
}

}  // namespace

EvalChat::EvalChat() = default;
EvalChat::EvalChat(EvalChat&&) = default;
EvalChat& EvalChat::operator=(EvalChat&&) = default;
EvalChat::~EvalChat() = default;

// static
bool LearnedMemoryEval::IsEnabled() {
  const auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch(kLearnedMemoryEvalChatsSwitch) &&
         command_line->HasSwitch(kLearnedMemoryEvalOutputSwitch);
}

// static
std::unique_ptr<LearnedMemoryEval> LearnedMemoryEval::CreateFromCommandLine() {
  if (!IsEnabled()) {
    return nullptr;
  }
  const auto* command_line = base::CommandLine::ForCurrentProcess();
  return std::make_unique<LearnedMemoryEval>(
      command_line->GetSwitchValuePath(kLearnedMemoryEvalChatsSwitch),
      command_line->GetSwitchValuePath(kLearnedMemoryEvalOutputSwitch));
}

// static
std::optional<std::vector<EvalChat>> LearnedMemoryEval::ParseChatSet(
    std::string_view json,
    base::Time now) {
  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  const base::ListValue* chats = root ? root->FindList("chats") : nullptr;
  if (!chats) {
    return std::nullopt;
  }
  std::vector<EvalChat> result;
  for (size_t c = 0; c < chats->size(); ++c) {
    const base::DictValue* chat = (*chats)[c].GetIfDict();
    const base::ListValue* turns = chat ? chat->FindList("turns") : nullptr;
    if (!turns || turns->empty()) {
      return std::nullopt;
    }
    const std::string uuid = base::StrCat({"eval-", base::NumberToString(c)});
    const std::string* title = chat->FindString("title");
    const base::Time start =
        now - base::Days(chat->FindDouble("days_ago").value_or(0));
    EvalChat eval_chat;
    for (size_t t = 0; t < turns->size(); ++t) {
      const base::DictValue* turn = (*turns)[t].GetIfDict();
      const std::string* user = turn ? turn->FindString("user") : nullptr;
      if (!user) {
        return std::nullopt;
      }
      const std::string* assistant = turn->FindString("assistant");
      const std::string prefix =
          base::StrCat({uuid, "-", base::NumberToString(t)});
      const base::Time date = start + kTurnGap * (2 * t);
      eval_chat.entries.push_back(MakeTurn(base::StrCat({prefix, "-user"}),
                                           mojom::CharacterType::HUMAN, *user,
                                           date));
      eval_chat.entries.push_back(MakeTurn(
          base::StrCat({prefix, "-assistant"}), mojom::CharacterType::ASSISTANT,
          assistant ? *assistant : std::string("OK."), date + kTurnGap));
    }
    eval_chat.conversation = mojom::Conversation::New(
        uuid, title ? *title : uuid, eval_chat.entries.back()->created_time,
        false, std::nullopt, 0, 0, false,
        std::vector<mojom::AssociatedContentPtr>());
    result.push_back(std::move(eval_chat));
  }
  return result;
}

LearnedMemoryEval::LearnedMemoryEval(base::FilePath chats_path,
                                     base::FilePath output_path)
    : chats_path_(std::move(chats_path)),
      output_path_(std::move(output_path)) {}

LearnedMemoryEval::~LearnedMemoryEval() = default;

void LearnedMemoryEval::Start(base::SequenceBound<AIChatDatabase>& db,
                              DreamingConfig config,
                              RunDreamingCallback run_dreaming) {
  db_ = &db;
  config_ = config;
  run_dreaming_ = std::move(run_dreaming);
  LOG(WARNING) << "Learned memory eval: importing " << chats_path_;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&ReadFile, chats_path_),
      base::BindOnce(&LearnedMemoryEval::OnChatsRead,
                     weak_ptr_factory_.GetWeakPtr()));
}

void LearnedMemoryEval::OnChatsRead(std::optional<std::string> json) {
  std::optional<std::vector<EvalChat>> chats =
      json ? ParseChatSet(*json, base::Time::Now()) : std::nullopt;
  if (!chats) {
    Fail("cannot read or parse the chat set");
    return;
  }
  chat_count_ = chats->size();
  // The database runs the calls in order, so GetAllConversations() runs
  // after all writes.
  for (auto& chat : *chats) {
    const std::string uuid = chat.conversation->uuid;
    auto first = std::move(chat.entries.front());
    db_->AsyncCall(base::IgnoreResult(&AIChatDatabase::AddConversation))
        .WithArgs(std::move(chat.conversation), std::vector<std::string>(),
                  std::move(first));
    for (size_t i = 1; i < chat.entries.size(); ++i) {
      db_->AsyncCall(base::IgnoreResult(&AIChatDatabase::AddConversationEntry))
          .WithArgs(uuid, std::move(chat.entries[i]), std::nullopt);
    }
  }
  db_->AsyncCall(&AIChatDatabase::GetAllConversations)
      .Then(base::BindOnce(&LearnedMemoryEval::OnImported,
                           weak_ptr_factory_.GetWeakPtr()));
}

void LearnedMemoryEval::OnImported(
    std::vector<mojom::ConversationPtr> conversations) {
  LOG(WARNING) << "Learned memory eval: " << conversations.size()
               << " chats in the database (" << chat_count_
               << " in the set). Dreaming starts.";
  std::move(run_dreaming_)
      .Run(base::BindOnce(&LearnedMemoryEval::OnDreamingDone,
                          weak_ptr_factory_.GetWeakPtr()));
}

void LearnedMemoryEval::OnDreamingDone(DreamingResult result) {
  db_->AsyncCall(&AIChatDatabase::GetAllLearnedMemories)
      .Then(base::BindOnce(&LearnedMemoryEval::OnMemories,
                           weak_ptr_factory_.GetWeakPtr(), std::move(result)));
}

void LearnedMemoryEval::OnMemories(DreamingResult result,
                                   std::vector<LearnedMemory> memories) {
  base::ListValue memory_list;
  for (const auto& memory : memories) {
    memory_list.Append(MemoryToDict(memory));
  }
  base::DictValue report;
  report.Set("chats_path", chats_path_.AsUTF8Unsafe());
  report.Set(
      "config",
      base::DictValue()
          .Set("certain_threshold", config_.certain_threshold)
          .Set("certain_margin", config_.certain_margin)
          .Set("time_limit_s", static_cast<int>(config_.time_limit.InSeconds()))
          .Set("decision_model",
               features::kLearnedMemoryDecisionModelName.Get())
          .Set("local_llm_model",
               features::kLearnedMemoryLocalLlmModelName.Get()));
  report.Set(
      "result",
      base::DictValue()
          .Set("status", DreamingStatusToString(result.status))
          .Set("turns_read", static_cast<int>(result.turns_read))
          .Set("turns_kept", static_cast<int>(result.turns_kept))
          .Set("memories_added", static_cast<int>(result.memories_added))
          .Set("memories_updated", static_cast<int>(result.memories_updated)));
  report.Set("trace", std::move(result.trace));
  report.Set("memories", std::move(memory_list));
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&WriteReport, output_path_, std::move(report)));
}

void LearnedMemoryEval::Fail(std::string_view error) {
  LOG(ERROR) << "Learned memory eval: " << error;
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&WriteReport, output_path_,
                     base::DictValue().Set("error", error)));
}

}  // namespace ai_chat
