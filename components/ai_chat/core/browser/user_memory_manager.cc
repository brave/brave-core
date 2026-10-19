// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/user_memory_manager.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/dreaming_text_utils.h"
#include "brave/components/ai_chat/core/browser/learned_memory_eval.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

DreamingRunRecord::DreamingRunRecord() = default;
DreamingRunRecord::DreamingRunRecord(DreamingRunRecord&&) = default;
DreamingRunRecord& DreamingRunRecord::operator=(DreamingRunRecord&&) = default;
DreamingRunRecord::~DreamingRunRecord() = default;

TurnLookupRecord::TurnLookupRecord() = default;
TurnLookupRecord::TurnLookupRecord(TurnLookupRecord&&) = default;
TurnLookupRecord& TurnLookupRecord::operator=(TurnLookupRecord&&) = default;
TurnLookupRecord::~TurnLookupRecord() = default;

MemoryEventRecord::MemoryEventRecord() = default;
MemoryEventRecord::MemoryEventRecord(MemoryEventRecord&&) = default;
MemoryEventRecord& MemoryEventRecord::operator=(MemoryEventRecord&&) = default;
MemoryEventRecord::~MemoryEventRecord() = default;

namespace {

// The title of each stored chat.
std::map<std::string, std::string> GetChatTitles(
    const std::vector<mojom::ConversationPtr>& conversations) {
  std::map<std::string, std::string> titles;
  for (const mojom::ConversationPtr& conversation : conversations) {
    titles[conversation->uuid] = conversation->title;
  }
  return titles;
}

// The chats of |links|, oldest link first: each chat at its newest link, and
// the chat of the newest link last. A chat that is not in |titles| is left
// out.
std::vector<LearnedMemoryChat> GetChatsOfLinks(
    const std::vector<MemorySourceLink>& links,
    const std::map<std::string, std::string>& titles) {
  std::vector<LearnedMemoryChat> chats;
  for (const MemorySourceLink& link : links) {
    auto title = titles.find(link.conversation_uuid);
    if (title == titles.end()) {
      continue;
    }
    std::erase_if(chats, [&link](const LearnedMemoryChat& chat) {
      return chat.conversation_uuid == link.conversation_uuid;
    });
    chats.push_back({link.conversation_uuid, link.entry_uuid, title->second});
  }
  return chats;
}

}  // namespace

// The sentences of one memory, while its chats are read one at a time.
struct UserMemoryManager::SourcesRequest {
  std::vector<MemorySourceLink> links;
  LearnedMemorySourcesCallback callback;
  std::map<std::string, std::string> titles;
  // The chats that are left to read.
  std::vector<std::string> chats_to_read;
  // The learnable text and the date of each message that a link names.
  std::map<std::string, std::pair<std::string, base::Time>> messages;
};

// static
DreamingConfig UserMemoryManager::GetDreamingConfigFromFeatures() {
  DreamingConfig config;
  config.certain_threshold = features::kLearnedMemoryCertainThreshold.Get();
  config.certain_margin = features::kLearnedMemoryCertainMargin.Get();
  config.gate_threshold = features::kLearnedMemoryGateThreshold.Get();
  config.fact_threshold = features::kLearnedMemoryFactThreshold.Get();
  config.max_sensitive = features::kLearnedMemoryMaxSensitive.Get();
  config.max_instruction = features::kLearnedMemoryMaxInstruction.Get();
  config.max_short_lived = features::kLearnedMemoryMaxShortLived.Get();
  config.max_neighbors = features::kLearnedMemoryMaxNeighbors.Get();
  config.min_neighbor_similarity =
      static_cast<float>(features::kLearnedMemoryMinNeighborSimilarity.Get());
  config.time_limit = features::kLearnedMemoryRunTimeLimit.Get();
  config.max_writing_requests =
      features::kLearnedMemoryMaxRewriteRequests.Get();
  config.max_relation_requests =
      features::kLearnedMemoryMaxRelationRequests.Get();
  // brave://leo-memory-internals shows the trace of each run.
  config.record_trace = true;
  // The eval reads what the run stored, and has no user to review it.
  config.review_changes = !LearnedMemoryEval::IsEnabled();
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

void UserMemoryManager::SetLearnedMemorySearch(
    base::WeakPtr<LearnedMemorySearch> search) {
  search_ = std::move(search);
  MaybeStartEval();
}

void UserMemoryManager::OnStorageReady() {
  if (!data_source_->IsStorageReady()) {
    return;
  }
  if (LearnedMemoryEval::IsEnabled()) {
    MaybeStartEval();
    return;
  }
  ScheduleNextDailyDreaming();
  if (!expiry_timer_.IsRunning()) {
    ScheduleExpiry(kFirstRunDelay);
  }
}

void UserMemoryManager::MaybeStartEval() {
  if (!LearnedMemoryEval::IsEnabled() || eval_ || !search_ ||
      !data_source_->IsStorageReady()) {
    return;
  }
  eval_ = LearnedMemoryEval::CreateFromCommandLine();
  eval_->Start(*data_source_, config_,
               base::BindOnce(&UserMemoryManager::LearnFromChats,
                              base::Unretained(this)));
}

void UserMemoryManager::OnAllConversationsDeleted() {
  // The chats and the memories are gone, so the run has nothing to continue.
  // Cancel it first: its callback sees the end of the run.
  if (dreaming_run_) {
    dreaming_run_->Cancel();
  }
  pending_review_.reset();
  if (!data_source_->IsStorageReady()) {
    // Storage is off. OnStorageReady() starts everything again.
    dreaming_timer_.Stop();
    expiry_timer_.Stop();
    eval_.reset();
    return;
  }
  // Storage is on, and a new chat starts a new history. Dreaming goes on with
  // its schedule.
  if (!LearnedMemoryEval::IsEnabled() && !dreaming_timer_.IsRunning()) {
    ScheduleNextDailyDreaming();
  }
}

void UserMemoryManager::OnConversationDeleted(
    const std::string& conversation_uuid) {
  DropReviewWithSource([&](const MemorySourceLink& link) {
    return link.conversation_uuid == conversation_uuid;
  });
}

void UserMemoryManager::OnConversationEntryRemoved(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  DropReviewWithSource([&](const MemorySourceLink& link) {
    return link.entry_uuid == entry_uuid;
  });
}

void UserMemoryManager::DropReviewWithSource(
    base::FunctionRef<bool(const MemorySourceLink&)> matches) {
  if (!pending_review_) {
    return;
  }
  for (const DreamingProposal& proposal : pending_review_->proposals) {
    if (std::ranges::any_of(proposal.memory.links, matches) ||
        (proposal.memory.previous &&
         std::ranges::any_of(proposal.memory.previous->links, matches))) {
      pending_review_.reset();
      return;
    }
  }
}

void UserMemoryManager::LearnFromChats(DreamingCallback callback) {
  StartRun(std::move(callback), config_,
           LearnedMemoryEval::IsEnabled() ? DreamingTrigger::kEval
                                          : DreamingTrigger::kTimer);
}

void UserMemoryManager::DreamNow(DreamingCallback callback) {
  if (!prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled)) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kUnavailable));
    return;
  }
  DreamingConfig config = config_;
  config.time_limit = std::max(config.time_limit, kDreamNowTimeLimit);
  StartRun(std::move(callback), config, DreamingTrigger::kDreamNow);
}

void UserMemoryManager::GetLearnedMemories(LearnedMemoriesCallback callback) {
  data_source_->GetLearnedMemories(std::move(callback));
}

void UserMemoryManager::GetLearnedMemoriesWithChats(
    LearnedMemoriesWithChatsCallback callback) {
  data_source_->GetLearnedMemories(
      base::BindOnce(&UserMemoryManager::OnMemoriesForChats,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void UserMemoryManager::OnMemoriesForChats(
    LearnedMemoriesWithChatsCallback callback,
    std::vector<LearnedMemory> memories) {
  if (memories.empty()) {
    std::move(callback).Run({}, {});
    return;
  }
  data_source_->GetStoredConversations(base::BindOnce(
      [](std::vector<LearnedMemory> memories,
         LearnedMemoriesWithChatsCallback callback,
         std::vector<mojom::ConversationPtr> conversations) {
        const std::map<std::string, std::string> titles =
            GetChatTitles(conversations);
        std::vector<std::vector<LearnedMemoryChat>> chats;
        chats.reserve(memories.size());
        for (const LearnedMemory& memory : memories) {
          chats.push_back(GetChatsOfLinks(memory.links, titles));
        }
        std::move(callback).Run(std::move(memories), std::move(chats));
      },
      std::move(memories), std::move(callback)));
}

void UserMemoryManager::GetLearnedMemorySources(
    const std::string& uuid,
    LearnedMemorySourcesCallback callback) {
  // The data source gives the sources only with all memories. They are few.
  data_source_->GetLearnedMemories(base::BindOnce(
      &UserMemoryManager::OnMemoriesForSources, weak_ptr_factory_.GetWeakPtr(),
      uuid, std::move(callback)));
}

void UserMemoryManager::OnMemoriesForSources(
    const std::string& uuid,
    LearnedMemorySourcesCallback callback,
    std::vector<LearnedMemory> memories) {
  auto it = std::ranges::find(memories, uuid, &LearnedMemory::uuid);
  if (it == memories.end() || it->links.empty()) {
    std::move(callback).Run({});
    return;
  }
  auto request = std::make_unique<SourcesRequest>();
  request->links = std::move(it->links);
  request->callback = std::move(callback);
  data_source_->GetStoredConversations(
      base::BindOnce(&UserMemoryManager::OnConversationsForSources,
                     weak_ptr_factory_.GetWeakPtr(), std::move(request)));
}

void UserMemoryManager::OnConversationsForSources(
    std::unique_ptr<SourcesRequest> request,
    std::vector<mojom::ConversationPtr> conversations) {
  request->titles = GetChatTitles(conversations);
  // Each stored chat of the links, once.
  std::set<std::string> chats;
  for (const MemorySourceLink& link : request->links) {
    if (request->titles.contains(link.conversation_uuid) &&
        chats.insert(link.conversation_uuid).second) {
      request->chats_to_read.push_back(link.conversation_uuid);
    }
  }
  ReadNextSourceChat(std::move(request));
}

void UserMemoryManager::ReadNextSourceChat(
    std::unique_ptr<SourcesRequest> request) {
  if (request->chats_to_read.empty()) {
    // The sentence of a link is its place in the learnable text of the
    // message, as Dreaming split it.
    std::vector<DreamingSource> sources;
    for (const MemorySourceLink& link : request->links) {
      auto message = request->messages.find(link.entry_uuid);
      if (message == request->messages.end()) {
        continue;
      }
      std::vector<std::string> sentences =
          SplitIntoSentences(message->second.first);
      if (link.sentence_index >= sentences.size()) {
        continue;
      }
      sources.push_back({std::move(sentences[link.sentence_index]),
                         link.conversation_uuid, link.entry_uuid,
                         request->titles[link.conversation_uuid],
                         message->second.second});
    }
    std::move(request->callback).Run(std::move(sources));
    return;
  }
  std::string uuid = std::move(request->chats_to_read.back());
  request->chats_to_read.pop_back();
  data_source_->GetStoredConversationData(
      uuid, base::BindOnce(&UserMemoryManager::OnSourceChat,
                           weak_ptr_factory_.GetWeakPtr(), std::move(request)));
}

void UserMemoryManager::OnSourceChat(std::unique_ptr<SourcesRequest> request,
                                     mojom::ConversationArchivePtr archive) {
  if (archive) {
    std::set<std::string> linked;
    for (const MemorySourceLink& link : request->links) {
      linked.insert(link.entry_uuid);
    }
    for (const mojom::ConversationTurnPtr& entry : archive->entries) {
      if (!entry->uuid || !linked.contains(*entry->uuid)) {
        continue;
      }
      if (std::optional<std::string> text =
              DreamingRun::GetLearnableText(*entry)) {
        request->messages[*entry->uuid] = {std::move(*text),
                                           entry->created_time};
      }
    }
  }
  ReadNextSourceChat(std::move(request));
}

void UserMemoryManager::DeleteLearnedMemory(
    const std::string& uuid,
    base::OnceCallback<void(bool)> callback) {
  if (pending_review_) {
    std::erase_if(pending_review_->proposals,
                  [&](const DreamingProposal& proposal) {
                    return proposal.memory.uuid == uuid;
                  });
    if (pending_review_->proposals.empty()) {
      // Nothing is left to review: the turns are done.
      SetWatermarks(pending_review_->watermarks);
      pending_review_.reset();
    }
  }
  RecordMemoryEvent("deleted", uuid, "");
  data_source_->DeleteLearnedMemory(uuid, std::move(callback));
}

void UserMemoryManager::SetLearnedMemoryPermanent(
    const std::string& uuid,
    bool permanent,
    base::OnceCallback<void(bool)> callback) {
  // The writes of a review are on their way. One of them could undo this one.
  if (!data_source_->IsStorageReady() || applying_review_) {
    std::move(callback).Run(false);
    return;
  }
  // The data source has no call that changes only the type. Read the whole
  // memory, with its sources, and write it again.
  data_source_->GetLearnedMemories(base::BindOnce(
      &UserMemoryManager::OnMemoriesForPermanent,
      weak_ptr_factory_.GetWeakPtr(), uuid, permanent, std::move(callback)));
}

void UserMemoryManager::OnMemoriesForPermanent(
    const std::string& uuid,
    bool permanent,
    base::OnceCallback<void(bool)> callback,
    std::vector<LearnedMemory> memories) {
  auto it = std::ranges::find(memories, uuid, &LearnedMemory::uuid);
  if (it == memories.end() || applying_review_) {
    std::move(callback).Run(false);
    return;
  }
  if ((it->type == LearnedMemoryType::kPermanent) == permanent) {
    // Nothing to change. A short-term memory stays short-term when the user
    // turns the setting off twice.
    std::move(callback).Run(true);
    return;
  }
  LearnedMemory memory = std::move(*it);
  // A long life is the safe error, as for a type that is not certain.
  memory.type =
      permanent ? LearnedMemoryType::kPermanent : LearnedMemoryType::kLongTerm;
  RecordMemoryEvent(permanent ? "keep_forever" : "release_keep_forever",
                    memory.uuid, memory.text);
  // The review and the write go in the same task, so no review can start
  // between them.
  UpdateReviewForType(memory);
  data_source_->AddOrUpdateLearnedMemory(std::move(memory),
                                         std::move(callback));
}

void UserMemoryManager::UpdateReviewForType(const LearnedMemory& memory) {
  if (!pending_review_) {
    return;
  }
  auto it = std::ranges::find(
      pending_review_->proposals, memory.uuid,
      [](const DreamingProposal& proposal) { return proposal.memory.uuid; });
  if (it == pending_review_->proposals.end()) {
    return;
  }
  const bool permanent = memory.type == LearnedMemoryType::kPermanent;
  // The user keeps the text as it is, so a new text for it is not stored. The
  // review writes the other changes with the new type.
  const bool dropped =
      permanent && it->kind == DreamingProposal::Kind::kChanged;
  AppendRunStep(
      pending_review_run_id_,
      permanent ? "keep_forever" : "release_keep_forever",
      base::DictValue()
          .Set("uuid", memory.uuid)
          .Set("text", memory.text)
          .Set("pending_change", DreamingProposalKindToString(it->kind))
          .Set("pending_change_dropped", dropped)
          .Set("pending_text",
               dropped ? base::Value(it->memory.text) : base::Value()));
  if (dropped) {
    pending_review_->proposals.erase(it);
    if (pending_review_->proposals.empty()) {
      // Nothing is left to review: the turns are done.
      SetWatermarks(pending_review_->watermarks);
      pending_review_.reset();
    }
    return;
  }
  it->memory.type = memory.type;
  if (it->stored) {
    it->stored->type = memory.type;
  }
}

void UserMemoryManager::MarkLearnedMemoriesUsed(std::vector<std::string> uuids,
                                                std::string_view used_by) {
  if (uuids.empty() || !data_source_->IsStorageReady()) {
    return;
  }
  // The dates are read first, to write only the memories that are due.
  data_source_->GetLearnedMemoriesByUuid(
      std::move(uuids),
      base::BindOnce(&UserMemoryManager::OnMemoriesToMarkUsed,
                     weak_ptr_factory_.GetWeakPtr(), std::string(used_by)));
}

void UserMemoryManager::OnMemoriesToMarkUsed(
    std::string used_by,
    std::vector<LearnedMemory> memories) {
  const base::Time now = base::Time::Now();
  std::vector<std::string> due;
  base::ListValue marked;
  for (const LearnedMemory& memory : memories) {
    // A kept memory does not expire. A memory used a short time ago has a date
    // that is fresh enough.
    if (memory.type == LearnedMemoryType::kPermanent ||
        (!memory.last_used_date.is_null() &&
         now - memory.last_used_date < kUseGranularity)) {
      continue;
    }
    due.push_back(memory.uuid);
    LearnedMemory used = memory;
    used.last_used_date = now;
    marked.Append(
        base::DictValue()
            .Set("uuid", memory.uuid)
            .Set("text", memory.text)
            .Set("expires_before",
                 base::TimeFormatAsIso8601(
                     GetLearnedMemoryExpiryTime(memory).value_or(base::Time())))
            .Set("expires_now",
                 base::TimeFormatAsIso8601(
                     GetLearnedMemoryExpiryTime(used).value_or(base::Time()))));
  }
  if (due.empty()) {
    return;
  }
  RecordMemoryEvent(
      "used", "", "",
      base::DictValue().Set("by", used_by).Set("memories", std::move(marked)));
  data_source_->MarkLearnedMemoriesUsed(std::move(due), now, base::DoNothing());
}

void UserMemoryManager::ExpireLearnedMemories() {
  // A run in progress reads the memories. Its review is safe, but the clean-up
  // waits for the next time.
  if (!data_source_->IsStorageReady() || dreaming_run_ || applying_review_) {
    return;
  }
  data_source_->GetLearnedMemories(base::BindOnce(
      &UserMemoryManager::OnMemoriesToExpire, weak_ptr_factory_.GetWeakPtr()));
}

void UserMemoryManager::OnMemoriesToExpire(
    std::vector<LearnedMemory> memories) {
  if (dreaming_run_ || applying_review_) {
    return;
  }
  const base::Time now = base::Time::Now();
  for (const LearnedMemory& memory : memories) {
    std::optional<base::Time> expiry = GetLearnedMemoryExpiryTime(memory);
    if (!expiry || *expiry > now) {
      continue;
    }
    // The same task drops the change of the review and deletes the memory, so
    // the review cannot write the memory back.
    const bool dropped = DropPendingChange(
        memory.uuid, "expired",
        base::DictValue().Set("uuid", memory.uuid).Set("text", memory.text));
    RecordMemoryEvent(
        "expired", memory.uuid, memory.text,
        base::DictValue()
            .Set("type", LearnedMemoryTypeToString(memory.type))
            .Set("last_used", base::TimeFormatAsIso8601(memory.last_used_date))
            .Set("expired_at", base::TimeFormatAsIso8601(*expiry))
            .Set("pending_change_dropped", dropped));
    data_source_->DeleteLearnedMemory(memory.uuid, base::DoNothing());
  }
}

void UserMemoryManager::ScheduleExpiry(base::TimeDelta delay) {
  if (!data_source_->IsStorageReady() || LearnedMemoryEval::IsEnabled()) {
    return;
  }
  expiry_timer_.Start(FROM_HERE, delay,
                      base::BindOnce(&UserMemoryManager::OnExpiryTimer,
                                     base::Unretained(this)));
}

void UserMemoryManager::OnExpiryTimer() {
  if (dreaming_run_ || applying_review_) {
    // A run reads the memories. Try again soon.
    ScheduleExpiry(base::Minutes(10));
    return;
  }
  ExpireLearnedMemories();
  ScheduleExpiry(kExpiryInterval);
}

void UserMemoryManager::EditLearnedMemory(const std::string& uuid,
                                          const std::string& text,
                                          EditLearnedMemoryCallback callback) {
  std::string trimmed(base::TrimWhitespaceASCII(text, base::TRIM_ALL));
  if (trimmed.empty() ||
      base::UTF8ToUTF16(trimmed).size() > kMaxMemoryTextLength) {
    std::move(callback).Run(LearnedMemoryEditError::kInvalidLength);
    return;
  }
  // The writes of a review are on their way. One of them could undo this one.
  if (!data_source_->IsStorageReady() || applying_review_) {
    std::move(callback).Run(LearnedMemoryEditError::kNotFound);
    return;
  }
  // The data source has no call that changes only the text. Read the whole
  // memory, with its sources, and write it again.
  data_source_->GetLearnedMemories(base::BindOnce(
      &UserMemoryManager::OnMemoriesForEdit, weak_ptr_factory_.GetWeakPtr(),
      uuid, std::move(trimmed), std::move(callback)));
}

void UserMemoryManager::OnMemoriesForEdit(const std::string& uuid,
                                          std::string text,
                                          EditLearnedMemoryCallback callback,
                                          std::vector<LearnedMemory> memories) {
  auto it = std::ranges::find(memories, uuid, &LearnedMemory::uuid);
  if (it == memories.end() || applying_review_) {
    std::move(callback).Run(LearnedMemoryEditError::kNotFound);
    return;
  }
  if (it->text == text) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  LearnedMemory memory = std::move(*it);
  const std::string old_text = memory.text;
  memory.previous = PreviousMemoryText{old_text, memory.links};
  memory.text = std::move(text);
  // The user's own change is a fresh use. The type stays: an edit does not
  // keep the memory as it is.
  memory.updated_date = base::Time::Now();
  memory.last_used_date = memory.updated_date;
  // The user's text wins over a change of the review for the same memory. The
  // review and the write go in the same task, so no review can start between
  // them.
  const bool dropped = DropPendingChange(
      memory.uuid, "edited_by_user",
      base::DictValue().Set("uuid", memory.uuid).Set("text", memory.text));
  RecordMemoryEvent("edited", memory.uuid, memory.text,
                    base::DictValue()
                        .Set("before", old_text)
                        .Set("pending_change_dropped", dropped));
  // The database adds 1 to the text version, so the index embeds the new text.
  data_source_->AddOrUpdateLearnedMemory(
      std::move(memory),
      base::BindOnce(
          [](EditLearnedMemoryCallback callback, bool success) {
            std::move(callback).Run(
                success ? std::nullopt
                        : std::optional(LearnedMemoryEditError::kNotFound));
          },
          std::move(callback)));
}

bool UserMemoryManager::DropPendingChange(const std::string& uuid,
                                          std::string_view step,
                                          base::DictValue data) {
  if (!pending_review_) {
    return false;
  }
  auto it = std::ranges::find(
      pending_review_->proposals, uuid,
      [](const DreamingProposal& proposal) { return proposal.memory.uuid; });
  if (it == pending_review_->proposals.end()) {
    return false;
  }
  data.Set("pending_change", DreamingProposalKindToString(it->kind));
  data.Set("pending_text", it->memory.text);
  AppendRunStep(pending_review_run_id_, step, std::move(data));
  pending_review_->proposals.erase(it);
  if (pending_review_->proposals.empty()) {
    // Nothing is left to review: the turns are done.
    SetWatermarks(pending_review_->watermarks);
    pending_review_.reset();
  }
  return true;
}

void UserMemoryManager::RecordMemoryEvent(std::string_view action,
                                          std::string_view uuid,
                                          std::string_view text,
                                          base::DictValue details) {
  MemoryEventRecord record;
  record.id = next_event_id_++;
  record.time = base::Time::Now();
  record.action = std::string(action);
  record.uuid = std::string(uuid);
  record.text = std::string(text);
  record.details = std::move(details);
  event_records_.push_front(std::move(record));
  if (event_records_.size() > kMaxMemoryEventRecords) {
    event_records_.pop_back();
  }
}

std::vector<MemoryEventRecord> UserMemoryManager::GetMemoryEventRecords()
    const {
  std::vector<MemoryEventRecord> records;
  for (const MemoryEventRecord& record : event_records_) {
    MemoryEventRecord copy;
    copy.id = record.id;
    copy.time = record.time;
    copy.action = record.action;
    copy.uuid = record.uuid;
    copy.text = record.text;
    copy.details = record.details.Clone();
    records.push_back(std::move(copy));
  }
  return records;
}

void UserMemoryManager::AppendRunStep(uint64_t run_id,
                                      std::string_view step,
                                      base::DictValue data) {
  auto it = std::ranges::find(run_records_, run_id, &DreamingRunRecord::id);
  if (it == run_records_.end()) {
    return;
  }
  data.Set("step", step);
  data.Set("t_ms", static_cast<int>(
                       (base::Time::Now() - it->start_time).InMilliseconds()));
  it->trace.Append(std::move(data));
}

void UserMemoryManager::DeleteAllLearnedMemories(
    base::OnceCallback<void(bool)> callback) {
  if (dreaming_run_) {
    dreaming_run_->Cancel();
    // A canceled run does not schedule the next one.
    if (!LearnedMemoryEval::IsEnabled() && !dreaming_timer_.IsRunning()) {
      ScheduleNextDailyDreaming();
    }
  }
  if (pending_review_) {
    // The watermarks move, so the discarded changes do not come back.
    SetWatermarks(pending_review_->watermarks);
    pending_review_.reset();
  }
  RecordMemoryEvent("deleted_all", "", "");
  data_source_->DeleteAllLearnedMemories(std::move(callback));
}

void UserMemoryManager::ApplyDreamingReview(
    std::vector<DreamingReviewDecision> kept,
    base::OnceCallback<void(bool)> callback) {
  if (!pending_review_ || !data_source_->IsStorageReady()) {
    std::move(callback).Run(false);
    return;
  }
  std::vector<DreamingProposal> proposals;
  for (const DreamingReviewDecision& decision : kept) {
    auto it = std::ranges::find(
        pending_review_->proposals, decision.uuid,
        [](const DreamingProposal& proposal) { return proposal.memory.uuid; });
    if (it == pending_review_->proposals.end()) {
      continue;
    }
    DreamingProposal proposal = *it;
    if (decision.text) {
      std::string text(
          base::TrimWhitespaceASCII(*decision.text, base::TRIM_ALL));
      if (text.empty() ||
          base::UTF8ToUTF16(text).size() > kMaxMemoryTextLength) {
        std::move(callback).Run(false);
        return;
      }
      if (text != proposal.memory.text) {
        if (proposal.stored && !proposal.memory.previous) {
          proposal.memory.previous =
              PreviousMemoryText{proposal.stored->text, proposal.stored->links};
        }
        proposal.memory.text = std::move(text);
      }
    }
    proposals.push_back(std::move(proposal));
  }
  const size_t discarded = pending_review_->proposals.size() - proposals.size();
  const uint64_t run_id = pending_review_run_id_;
  std::map<std::string, base::Time> watermarks =
      std::move(pending_review_->watermarks);
  pending_review_.reset();
  applying_review_ = true;
  // A memory can change after the run, for example by a delete. Read the text
  // versions before the writes.
  data_source_->GetLearnedMemoryStamps(base::BindOnce(
      &UserMemoryManager::OnStampsForReview, weak_ptr_factory_.GetWeakPtr(),
      std::move(proposals), run_id, discarded, std::move(watermarks),
      std::move(callback)));
}

void UserMemoryManager::OnStampsForReview(
    std::vector<DreamingProposal> kept,
    uint64_t run_id,
    size_t discarded,
    std::map<std::string, base::Time> watermarks,
    base::OnceCallback<void(bool)> callback,
    std::vector<LearnedMemoryStamp> stamps) {
  std::map<std::string, int> versions;
  for (const LearnedMemoryStamp& stamp : stamps) {
    versions[stamp.uuid] = stamp.text_version;
  }
  std::vector<LearnedMemory> writes;
  base::ListValue written;
  base::ListValue skipped;
  for (DreamingProposal& proposal : kept) {
    auto it = versions.find(proposal.memory.uuid);
    const bool unchanged = proposal.stored
                               ? it != versions.end() &&
                                     it->second == proposal.stored->text_version
                               : it == versions.end();
    base::DictValue entry =
        base::DictValue()
            .Set("kind", DreamingProposalKindToString(proposal.kind))
            .Set("uuid", proposal.memory.uuid)
            .Set("type", LearnedMemoryTypeToString(proposal.memory.type))
            .Set("text", proposal.memory.text);
    if (unchanged) {
      written.Append(std::move(entry));
      // A new memory, a change, and a fact that came again all start the
      // count of the expiry now. The date of the chat can be old.
      proposal.memory.last_used_date = base::Time::Now();
      writes.push_back(std::move(proposal.memory));
    } else {
      skipped.Append(std::move(entry));
    }
  }
  AppendRunStep(run_id, "review_applied",
                base::DictValue()
                    .Set("written", std::move(written))
                    // The memory changed or went away after the run.
                    .Set("skipped_changed_since_run", std::move(skipped))
                    .Set("discarded_by_user", static_cast<int>(discarded)));
  auto barrier = base::BarrierCallback<bool>(
      writes.size() + watermarks.size(),
      base::BindOnce(&UserMemoryManager::OnReviewApplied,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  for (LearnedMemory& memory : writes) {
    data_source_->AddOrUpdateLearnedMemory(std::move(memory), barrier);
  }
  for (const auto& [conversation_uuid, date] : watermarks) {
    data_source_->SetMemoryWatermark(conversation_uuid, date, barrier);
  }
}

void UserMemoryManager::OnReviewApplied(base::OnceCallback<void(bool)> callback,
                                        std::vector<bool> results) {
  applying_review_ = false;
  std::move(callback).Run(
      std::ranges::all_of(results, [](bool success) { return success; }));
}

void UserMemoryManager::SetWatermarks(
    const std::map<std::string, base::Time>& watermarks) {
  for (const auto& [conversation_uuid, date] : watermarks) {
    data_source_->SetMemoryWatermark(conversation_uuid, date,
                                     base::DoNothing());
  }
}

void UserMemoryManager::GetMemoriesForTurn(
    const std::string& conversation_uuid,
    std::vector<std::string> user_messages,
    TurnMemoriesCallback callback) {
  if (!data_source_->IsStorageReady() ||
      !prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled) ||
      user_messages.empty()) {
    std::move(callback).Run({});
    return;
  }
  const uint64_t id = next_turn_lookup_id_++;
  TurnLookupRecord record;
  record.id = id;
  record.conversation_uuid = conversation_uuid;
  record.start_time = base::Time::Now();
  turn_records_.push_front(std::move(record));
  if (turn_records_.size() > kMaxTurnLookupRecords) {
    turn_records_.pop_back();
  }
  auto lookup = std::make_unique<TurnMemoryLookup>(
      *data_source_, search_, *decision_client_, turn_config_,
      std::move(user_messages),
      base::BindOnce(&UserMemoryManager::OnTurnLookupDone,
                     weak_ptr_factory_.GetWeakPtr(), id, std::move(callback)),
      base::BindOnce(&UserMemoryManager::OnTurnExchange,
                     weak_ptr_factory_.GetWeakPtr(), id));
  TurnMemoryLookup* lookup_ptr = lookup.get();
  turn_lookups_[id] = std::move(lookup);
  lookup_ptr->Start();
}

TurnLookupRecord* UserMemoryManager::FindTurnRecord(uint64_t lookup_id) {
  auto it = std::ranges::find(turn_records_, lookup_id, &TurnLookupRecord::id);
  return it == turn_records_.end() ? nullptr : &*it;
}

void UserMemoryManager::OnTurnExchange(
    uint64_t lookup_id,
    MemoryDecisionClient::Exchange exchange) {
  TurnLookupRecord* record = FindTurnRecord(lookup_id);
  if (!record) {
    return;
  }
  record->decision_requests.Append(
      base::DictValue()
          .Set("request", std::move(exchange.request))
          .Set("response", exchange.response
                               ? base::Value(std::move(*exchange.response))
                               : base::Value())
          .Set("error", std::move(exchange.error))
          .Set("latency_ms",
               static_cast<int>(exchange.latency.InMilliseconds()))
          .Set("arrived_ms",
               static_cast<int>(
                   (base::Time::Now() - record->start_time).InMilliseconds())));
}

std::vector<TurnLookupRecord> UserMemoryManager::GetTurnLookupRecords() const {
  std::vector<TurnLookupRecord> records;
  for (const TurnLookupRecord& record : turn_records_) {
    TurnLookupRecord copy;
    copy.id = record.id;
    copy.conversation_uuid = record.conversation_uuid;
    copy.start_time = record.start_time;
    copy.end_time = record.end_time;
    copy.decision_requests = record.decision_requests.Clone();
    auto lookup = turn_lookups_.find(record.id);
    copy.trace = lookup != turn_lookups_.end() ? lookup->second->trace().Clone()
                                               : record.trace.Clone();
    records.push_back(std::move(copy));
  }
  return records;
}

void UserMemoryManager::OnTurnLookupDone(
    uint64_t lookup_id,
    TurnMemoriesCallback callback,
    EngineConsumer::LearnedMemories memories) {
  // The lookup calls this as its last step, and it is still on the stack.
  // Delete it later.
  auto it = turn_lookups_.find(lookup_id);
  if (it != turn_lookups_.end()) {
    if (TurnLookupRecord* record = FindTurnRecord(lookup_id)) {
      record->end_time = base::Time::Now();
      record->trace = it->second->trace().Clone();
    }
    // The memories that go to the model are used.
    MarkLearnedMemoriesUsed(it->second->used_uuids(), "chat_turn");
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(it->second));
    turn_lookups_.erase(it);
  }
  std::move(callback).Run(std::move(memories));
}

std::vector<DreamingRunRecord> UserMemoryManager::GetDreamingRunRecords()
    const {
  std::vector<DreamingRunRecord> records;
  auto copy = [](const DreamingRunRecord& record) {
    DreamingRunRecord result;
    result.id = record.id;
    result.trigger = record.trigger;
    result.start_time = record.start_time;
    result.end_time = record.end_time;
    result.status = record.status;
    result.trace = record.trace.Clone();
    return result;
  };
  if (active_run_record_ && dreaming_run_) {
    records.push_back(copy(*active_run_record_));
    records.back().trace = dreaming_run_->trace().Clone();
  }
  for (const DreamingRunRecord& record : run_records_) {
    records.push_back(copy(record));
  }
  return records;
}

void UserMemoryManager::StartRun(DreamingCallback callback,
                                 const DreamingConfig& config,
                                 DreamingTrigger trigger) {
  if (dreaming_run_) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kBusy));
    return;
  }
  if (pending_review_ || applying_review_) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kReviewPending));
    return;
  }
  if (!data_source_->IsStorageReady() || !search_) {
    std::move(callback).Run(DreamingResult(DreamingStatus::kUnavailable));
    return;
  }
  dreaming_timer_.Stop();
  dreaming_callback_ = std::move(callback);
  run_llm_engine_ = llm_engine_factory_.Run();
  VLOG(1) << "Dreaming starts, local LLM: " << (run_llm_engine_ ? "yes" : "no");
  dreaming_run_ = std::make_unique<DreamingRun>(
      *data_source_, search_, *decision_client_, run_llm_engine_.get(),
      *embedder_, config,
      base::BindOnce(&UserMemoryManager::OnDreamingDone,
                     base::Unretained(this)));
  active_run_record_.emplace();
  active_run_record_->id = next_run_id_++;
  active_run_record_->trigger = trigger;
  active_run_record_->start_time = base::Time::Now();
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
  if (!search_ || pending_review_ || applying_review_) {
    // The embeddings service is not there (yet), or the user did not review
    // the last run. Check again later.
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

  uint64_t run_id = 0;
  if (active_run_record_) {
    DreamingRunRecord record = std::move(*active_run_record_);
    active_run_record_.reset();
    run_id = record.id;
    record.end_time = base::Time::Now();
    record.status = result.status;
    record.trace = result.trace.Clone();
    run_records_.push_front(std::move(record));
    if (run_records_.size() > kMaxDreamingRunRecords) {
      run_records_.pop_back();
    }
  }

  if (result.review) {
    if (result.review->proposals.empty()) {
      // Nothing to review: the turns are done.
      SetWatermarks(result.review->watermarks);
    } else {
      pending_review_ = std::move(result.review);
      pending_review_run_id_ = run_id;
    }
    result.review.reset();
  }

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
    case DreamingStatus::kReviewPending:
      // Storage is gone, or the chats or the memories were deleted.
      // OnStorageReady(), OnAllConversationsDeleted() and
      // DeleteAllLearnedMemories() schedule again.
      break;
  }
  // Reset first, so the callback can start a new run.
  DreamingCallback callback = std::move(dreaming_callback_);
  std::move(callback).Run(std::move(result));
}

}  // namespace ai_chat
