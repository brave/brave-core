// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/check_deref.h"
#include "base/containers/flat_set.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/browser/embeddings/passage_splitter.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_service.h"
#include "sql/database.h"

namespace ai_chat {

namespace {

using passage_embeddings::ComputeEmbeddingsStatus;
using passage_embeddings::PassagePriority;

// Passages follow those of semantic history search: up to 100 words, so each
// covers little enough to score well against a query about it, or 750 bytes of
// a script written without spaces.
constexpr size_t kMaxWordsPerPassage = 100;
constexpr size_t kMaxBytesPerPassage = 750;
// As in semantic history search, shorter passages say too little to be scored.
// Only space-separated text is held to it, since words can't be counted in a
// script written without spaces.
constexpr size_t kMinWordsPerPassage = 5;
// Bounds what a long response costs to index.
constexpr size_t kMaxPassagesPerEntry = 10;
constexpr size_t kMaxPassagesPerConversation = 3;
// Bump to compute every embedding again when the way passages are prepared
// changes.
constexpr int kPassageVersion = 2;
// For a model whose metadata gives no threshold.
constexpr float kDefaultMinScore = 0.45f;

// The text of the latest version of `entry`: what the user wrote, or the
// assistant's response.
std::string GetEntryText(const mojom::ConversationTurn& entry) {
  const mojom::ConversationTurn& latest =
      entry.edits && !entry.edits->empty() ? *entry.edits->back() : entry;
  if (latest.character_type == mojom::CharacterType::HUMAN) {
    return latest.text;
  }
  std::vector<std::string_view> completions;
  if (latest.events) {
    for (const mojom::ConversationEntryEventPtr& event : *latest.events) {
      if (event->is_completion_event()) {
        completions.push_back(event->get_completion_event()->completion);
      }
    }
  }
  return base::JoinString(completions, "\n\n");
}

// Appends the passages of `text`, from the entry with `entry_uuid`, which is
// empty for the conversation's title.
void AppendPassages(std::string_view text,
                    const std::string& entry_uuid,
                    const std::optional<std::string>& thread_uuid,
                    base::Time created_time,
                    std::vector<ConversationPassage>& passages) {
  std::vector<std::string> texts =
      SplitIntoPassages(text, kMaxWordsPerPassage, kMaxBytesPerPassage);
  std::erase_if(texts, [](const std::string& passage_text) {
    return base::IsStringASCII(passage_text) &&
           base::SplitStringPiece(passage_text, base::kWhitespaceASCII,
                                  base::TRIM_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY)
                   .size() < kMinWordsPerPassage;
  });
  texts.resize(std::min(texts.size(), kMaxPassagesPerEntry));
  for (std::string& passage_text : texts) {
    passages.emplace_back(entry_uuid, thread_uuid, created_time,
                          std::move(passage_text), std::vector<float>());
  }
}

std::vector<std::string> MakeDocuments(
    const std::vector<ConversationPassage>& passages) {
  return base::ToVector(passages, &ConversationPassage::text);
}

// Gives each of `passages` its embedding. Returns false when they couldn't be
// computed.
bool SetEmbeddings(std::vector<ConversationPassage>& passages,
                   std::vector<passage_embeddings::Embedding> embeddings,
                   ComputeEmbeddingsStatus status) {
  if (status != ComputeEmbeddingsStatus::kSuccess ||
      embeddings.size() != passages.size()) {
    return false;
  }
  for (size_t i = 0; i < passages.size(); ++i) {
    passages[i].embedding = embeddings[i].GetData();
  }
  return true;
}

}  // namespace

ConversationSearchResult::ConversationSearchResult() = default;
ConversationSearchResult::ConversationSearchResult(ConversationSearchResult&&) =
    default;
ConversationSearchResult& ConversationSearchResult::operator=(
    ConversationSearchResult&&) = default;
ConversationSearchResult::~ConversationSearchResult() = default;

AIChatEmbeddingsService::AIChatEmbeddingsService(
    AIChatService* ai_chat_service,
    PrefService* prefs,
    os_crypt_async::OSCryptAsync* os_crypt_async,
    passage_embeddings::Embedder* embedder,
    passage_embeddings::EmbedderMetadataProvider* embedder_metadata_provider,
    const base::FilePath& profile_path)
    : db_file_path_(profile_path.Append(kAIChatEmbeddingsDatabaseFileName)),
      db_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      ai_chat_service_(CHECK_DEREF(ai_chat_service)),
      prefs_(CHECK_DEREF(prefs)),
      embedder_(CHECK_DEREF(embedder)) {
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      prefs::kBraveAIChatUserMemories,
      base::BindRepeating(&AIChatEmbeddingsService::SyncMemories,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kBraveAIChatUserMemoryEnabled,
      base::BindRepeating(&AIChatEmbeddingsService::SyncMemories,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      local_ai::prefs::kBraveHistoryEmbeddingsEnabled,
      base::BindRepeating(&AIChatEmbeddingsService::OnSemanticSearchPrefChanged,
                          base::Unretained(this)));
  ai_chat_service_observation_.Observe(ai_chat_service);
  CHECK_DEREF(os_crypt_async)
      .GetInstance(base::BindOnce(&AIChatEmbeddingsService::OnEncryptorReady,
                                  weak_ptr_factory_.GetWeakPtr()));
  embedder_metadata_observation_.Observe(embedder_metadata_provider);
}

AIChatEmbeddingsService::~AIChatEmbeddingsService() = default;

// static
void AIChatEmbeddingsService::DeleteIndex(const base::FilePath& profile_path) {
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(base::IgnoreResult(&sql::Database::Delete),
                     profile_path.Append(kAIChatEmbeddingsDatabaseFileName)));
}

void AIChatEmbeddingsService::SearchConversations(
    const std::string& query,
    size_t count,
    const std::string& excluded_conversation_uuid,
    SearchConversationsCallback callback) {
  EmbedQuery(query, base::BindOnce(
                        &AIChatEmbeddingsService::OnConversationQueryEmbedded,
                        weak_ptr_factory_.GetWeakPtr(), count,
                        excluded_conversation_uuid, std::move(callback)));
}

void AIChatEmbeddingsService::SearchMemories(const std::string& query,
                                             size_t count,
                                             SearchMemoriesCallback callback) {
  if (!prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled)) {
    std::move(callback).Run({});
    return;
  }
  EmbedQuery(query,
             base::BindOnce(&AIChatEmbeddingsService::OnMemoryQueryEmbedded,
                            weak_ptr_factory_.GetWeakPtr(), count,
                            std::move(callback)));
}

base::WeakPtr<AIChatEmbeddingsService> AIChatEmbeddingsService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void AIChatEmbeddingsService::FlushForTesting(base::OnceClosure callback) {
  db_task_runner_->PostTaskAndReply(FROM_HERE, base::DoNothing(),
                                    std::move(callback));
}

bool AIChatEmbeddingsService::IsIndexingIdleForTesting() const {
  return reconciliations_in_progress_ == 0 && conversations_to_index_.empty() &&
         !indexing_conversation_uuid_ && entry_jobs_.empty() &&
         !memory_sync_pending_ && !memory_job_;
}

void AIChatEmbeddingsService::Shutdown() {
  ResetIndexing();
  query_jobs_.clear();
  ai_chat_service_observation_.Reset();
  embedder_metadata_observation_.Reset();
  pref_change_registrar_.RemoveAll();
  weak_ptr_factory_.InvalidateWeakPtrs();
  db_.Reset();
}

void AIChatEmbeddingsService::OnStorageReady() {
  MaybeReconcileConversations();
}

void AIChatEmbeddingsService::OnConversationEntryAdded(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry) {
  if (!db_ || !entry.uuid || DeferToConversationIndexing(conversation_uuid)) {
    return;
  }
  std::vector<ConversationPassage> passages;
  AppendPassages(GetEntryText(entry), *entry.uuid, entry.thread_uuid,
                 entry.created_time, passages);
  IndexEntryPassages(conversation_uuid, *entry.uuid, std::move(passages),
                     entry.created_time);
}

void AIChatEmbeddingsService::OnConversationEntryRemoved(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  if (!db_ || DeferToConversationIndexing(conversation_uuid)) {
    return;
  }
  entry_jobs_.erase({conversation_uuid, entry_uuid});
  db_.AsyncCall(
         base::IgnoreResult(&AIChatEmbeddingsDatabase::DeleteEntryPassages))
      .WithArgs(conversation_uuid, entry_uuid);
}

void AIChatEmbeddingsService::OnConversationTitleChanged(
    const std::string& conversation_uuid,
    const std::string& title) {
  if (!db_ || DeferToConversationIndexing(conversation_uuid)) {
    return;
  }
  std::vector<ConversationPassage> passages;
  AppendPassages(title, /*entry_uuid=*/"", /*thread_uuid=*/std::nullopt,
                 base::Time(), passages);
  IndexEntryPassages(conversation_uuid, /*entry_uuid=*/"", std::move(passages),
                     /*indexed_time=*/std::nullopt);
}

void AIChatEmbeddingsService::OnConversationDeleted(
    const std::string& conversation_uuid) {
  if (!db_) {
    return;
  }
  CancelEntryIndexing(conversation_uuid);
  std::erase(conversations_to_index_, conversation_uuid);
  if (indexing_conversation_uuid_ == conversation_uuid) {
    conversation_job_.reset();
    indexing_conversation_uuid_.reset();
    index_conversation_again_ = false;
  }
  db_.AsyncCall(
         base::IgnoreResult(&AIChatEmbeddingsDatabase::DeleteConversation))
      .WithArgs(conversation_uuid);
  MaybeIndexNextConversation();
}

void AIChatEmbeddingsService::OnAllConversationsDeleted() {
  if (!db_) {
    return;
  }
  entry_jobs_.clear();
  conversations_to_index_.clear();
  conversation_job_.reset();
  indexing_conversation_uuid_.reset();
  index_conversation_again_ = false;
  db_.AsyncCall(
      base::IgnoreResult(&AIChatEmbeddingsDatabase::DeleteAllConversations));
}

void AIChatEmbeddingsService::EmbedderMetadataUpdated(
    passage_embeddings::EmbedderMetadata metadata) {
  if (!metadata.IsValid()) {
    return;
  }
  const bool model_changed =
      embedder_metadata_ &&
      embedder_metadata_->model_version != metadata.model_version;
  embedder_metadata_ = metadata;
  if (model_changed && db_) {
    // The database deletes the previous model's embeddings when it opens for
    // the new one, and then everything is indexed again.
    ResetIndexing();
    db_.Reset();
  }
  MaybeCreateDatabase();
}

void AIChatEmbeddingsService::OnEncryptorReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  encryptor_ = std::move(encryptor);
  MaybeCreateDatabase();
}

void AIChatEmbeddingsService::MaybeCreateDatabase() {
  if (disabled_ || db_ || !encryptor_ || !embedder_metadata_) {
    return;
  }
  db_ = base::SequenceBound<AIChatEmbeddingsDatabase>(
      db_task_runner_, db_file_path_, embedder_metadata_->model_version,
      kPassageVersion, encryptor_);
  SyncMemories();
  MaybeReconcileConversations();
}

void AIChatEmbeddingsService::ResetIndexing() {
  indexing_weak_ptr_factory_.InvalidateWeakPtrs();
  reconciliations_in_progress_ = 0;
  conversations_to_index_.clear();
  indexing_conversation_uuid_.reset();
  index_conversation_again_ = false;
  conversation_job_.reset();
  entry_jobs_.clear();
  memory_sync_pending_ = false;
  memory_job_.reset();
}

void AIChatEmbeddingsService::OnSemanticSearchPrefChanged() {
  if (disabled_ ||
      prefs_->GetBoolean(local_ai::prefs::kBraveHistoryEmbeddingsEnabled)) {
    return;
  }
  disabled_ = true;
  ResetIndexing();
  query_jobs_.clear();
  ai_chat_service_observation_.Reset();
  embedder_metadata_observation_.Reset();
  pref_change_registrar_.RemoveAll();
  db_.Reset();
  // Runs on the database's sequence once it has closed.
  db_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(base::IgnoreResult(&sql::Database::Delete),
                                db_file_path_));
}

void AIChatEmbeddingsService::MaybeReconcileConversations() {
  if (!db_) {
    return;
  }
  if (!ai_chat_service_->IsAIChatHistoryEnabled()) {
    // Without storage no conversation is kept, so none stays indexed.
    db_.AsyncCall(
        base::IgnoreResult(&AIChatEmbeddingsDatabase::DeleteAllConversations));
    return;
  }
  // Until then only the conversations in memory are known, and OnStorageReady()
  // calls again.
  if (!ai_chat_service_->IsStorageReady()) {
    return;
  }
  // The index is read before the conversations, so that a conversation stored
  // in the meantime can't be taken for one that no longer exists.
  ++reconciliations_in_progress_;
  db_.AsyncCall(&AIChatEmbeddingsDatabase::GetIndexedConversations)
      .Then(base::BindOnce(&AIChatEmbeddingsService::OnGotIndexedConversations,
                           indexing_weak_ptr_factory_.GetWeakPtr()));
}

void AIChatEmbeddingsService::OnGotIndexedConversations(
    base::flat_map<std::string, base::Time> indexed_conversations) {
  ai_chat_service_->GetConversations(
      base::BindOnce(&AIChatEmbeddingsService::ReconcileConversations,
                     indexing_weak_ptr_factory_.GetWeakPtr(),
                     std::move(indexed_conversations)));
}

void AIChatEmbeddingsService::ReconcileConversations(
    base::flat_map<std::string, base::Time> indexed_conversations,
    std::vector<mojom::ConversationPtr> conversations) {
  --reconciliations_in_progress_;
  base::flat_set<std::string> stored_conversations;
  for (const mojom::ConversationPtr& conversation : conversations) {
    if (conversation->temporary || !conversation->has_content) {
      continue;
    }
    stored_conversations.insert(conversation->uuid);
    auto it = indexed_conversations.find(conversation->uuid);
    if (it == indexed_conversations.end() || it->second.is_null() ||
        it->second < conversation->updated_time) {
      QueueConversation(conversation->uuid);
    }
  }
  for (const auto& [uuid, indexed_time] : indexed_conversations) {
    if (!stored_conversations.contains(uuid)) {
      db_.AsyncCall(
             base::IgnoreResult(&AIChatEmbeddingsDatabase::DeleteConversation))
          .WithArgs(uuid);
    }
  }
  MaybeIndexNextConversation();
}

bool AIChatEmbeddingsService::DeferToConversationIndexing(
    const std::string& conversation_uuid) {
  if (indexing_conversation_uuid_ == conversation_uuid) {
    index_conversation_again_ = true;
    return true;
  }
  return std::ranges::contains(conversations_to_index_, conversation_uuid);
}

void AIChatEmbeddingsService::QueueConversation(
    const std::string& conversation_uuid) {
  if (!DeferToConversationIndexing(conversation_uuid)) {
    conversations_to_index_.push_back(conversation_uuid);
  }
}

void AIChatEmbeddingsService::MaybeIndexNextConversation() {
  if (!db_ || indexing_conversation_uuid_ || conversations_to_index_.empty()) {
    return;
  }
  indexing_conversation_uuid_ = std::move(conversations_to_index_.front());
  conversations_to_index_.pop_front();
  index_conversation_again_ = false;
  ai_chat_service_->GetConversations(base::BindOnce(
      &AIChatEmbeddingsService::OnGotConversationsForIndexing,
      indexing_weak_ptr_factory_.GetWeakPtr(), *indexing_conversation_uuid_));
}

void AIChatEmbeddingsService::OnGotConversationsForIndexing(
    const std::string& conversation_uuid,
    std::vector<mojom::ConversationPtr> conversations) {
  if (indexing_conversation_uuid_ != conversation_uuid) {
    return;
  }
  auto it = std::ranges::find_if(
      conversations, [&](const mojom::ConversationPtr& conversation) {
        return conversation->uuid == conversation_uuid;
      });
  if (it == conversations.end() || (*it)->temporary) {
    FinishConversationIndexing();
    return;
  }
  ai_chat_service_->GetAllConversationEntries(
      conversation_uuid,
      base::BindOnce(&AIChatEmbeddingsService::OnGotConversationEntries,
                     indexing_weak_ptr_factory_.GetWeakPtr(), conversation_uuid,
                     (*it)->title, (*it)->updated_time));
}

void AIChatEmbeddingsService::OnGotConversationEntries(
    const std::string& conversation_uuid,
    const std::string& title,
    base::Time updated_time,
    std::vector<mojom::ConversationTurnPtr> entries) {
  if (indexing_conversation_uuid_ != conversation_uuid) {
    return;
  }
  std::vector<ConversationPassage> passages;
  AppendPassages(title, /*entry_uuid=*/"", /*thread_uuid=*/std::nullopt,
                 base::Time(), passages);
  for (const mojom::ConversationTurnPtr& entry : entries) {
    if (entry->uuid) {
      AppendPassages(GetEntryText(*entry), *entry->uuid, entry->thread_uuid,
                     entry->created_time, passages);
    }
  }
  if (passages.empty()) {
    db_.AsyncCall(base::IgnoreResult(
                      &AIChatEmbeddingsDatabase::ReplaceConversationPassages))
        .WithArgs(conversation_uuid, std::move(passages), updated_time);
    FinishConversationIndexing();
    return;
  }
  std::vector<std::string> documents = MakeDocuments(passages);
  conversation_job_ = embedder_->ComputePassagesEmbeddings(
      PassagePriority::kPassive, std::move(documents),
      base::BindOnce(&AIChatEmbeddingsService::OnConversationEmbedded,
                     indexing_weak_ptr_factory_.GetWeakPtr(), conversation_uuid,
                     updated_time, std::move(passages)));
}

void AIChatEmbeddingsService::OnConversationEmbedded(
    const std::string& conversation_uuid,
    base::Time updated_time,
    std::vector<ConversationPassage> passages,
    std::vector<std::string> documents,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    ComputeEmbeddingsStatus status) {
  if (!conversation_job_ || conversation_job_->id() != job_id) {
    return;
  }
  conversation_job_.reset();
  // A conversation that couldn't be embedded keeps its previous index until
  // the next reconciliation.
  if (SetEmbeddings(passages, std::move(embeddings), status)) {
    db_.AsyncCall(base::IgnoreResult(
                      &AIChatEmbeddingsDatabase::ReplaceConversationPassages))
        .WithArgs(conversation_uuid, std::move(passages), updated_time);
  }
  FinishConversationIndexing();
}

void AIChatEmbeddingsService::FinishConversationIndexing() {
  std::optional<std::string> conversation_uuid =
      std::exchange(indexing_conversation_uuid_, std::nullopt);
  if (conversation_uuid && std::exchange(index_conversation_again_, false)) {
    conversations_to_index_.push_back(std::move(*conversation_uuid));
  }
  MaybeIndexNextConversation();
}

void AIChatEmbeddingsService::IndexEntryPassages(
    const std::string& conversation_uuid,
    const std::string& entry_uuid,
    std::vector<ConversationPassage> passages,
    std::optional<base::Time> indexed_time) {
  // Drops an earlier version of the entry that is still being embedded.
  entry_jobs_.erase({conversation_uuid, entry_uuid});
  if (passages.empty()) {
    db_.AsyncCall(
           base::IgnoreResult(&AIChatEmbeddingsDatabase::ReplaceEntryPassages))
        .WithArgs(conversation_uuid, entry_uuid, std::move(passages),
                  indexed_time);
    return;
  }
  std::vector<std::string> documents = MakeDocuments(passages);
  entry_jobs_.emplace(
      std::make_pair(conversation_uuid, entry_uuid),
      embedder_->ComputePassagesEmbeddings(
          PassagePriority::kPassive, std::move(documents),
          base::BindOnce(&AIChatEmbeddingsService::OnEntryEmbedded,
                         indexing_weak_ptr_factory_.GetWeakPtr(),
                         conversation_uuid, entry_uuid, indexed_time,
                         std::move(passages))));
}

void AIChatEmbeddingsService::OnEntryEmbedded(
    const std::string& conversation_uuid,
    const std::string& entry_uuid,
    std::optional<base::Time> indexed_time,
    std::vector<ConversationPassage> passages,
    std::vector<std::string> documents,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    ComputeEmbeddingsStatus status) {
  auto it = entry_jobs_.find({conversation_uuid, entry_uuid});
  if (it == entry_jobs_.end() || it->second.id() != job_id) {
    return;
  }
  entry_jobs_.erase(it);
  if (!SetEmbeddings(passages, std::move(embeddings), status)) {
    // Leaves the entry to be indexed with the rest of the conversation at the
    // next reconciliation.
    db_.AsyncCall(base::IgnoreResult(
                      &AIChatEmbeddingsDatabase::InvalidateConversation))
        .WithArgs(conversation_uuid);
    return;
  }
  db_.AsyncCall(
         base::IgnoreResult(&AIChatEmbeddingsDatabase::ReplaceEntryPassages))
      .WithArgs(conversation_uuid, entry_uuid, std::move(passages),
                indexed_time);
}

void AIChatEmbeddingsService::CancelEntryIndexing(
    const std::string& conversation_uuid) {
  auto begin = entry_jobs_.lower_bound({conversation_uuid, std::string()});
  auto end = begin;
  while (end != entry_jobs_.end() && end->first.first == conversation_uuid) {
    ++end;
  }
  entry_jobs_.erase(begin, end);
}

void AIChatEmbeddingsService::SyncMemories() {
  if (!db_) {
    return;
  }
  memory_job_.reset();
  ++memory_sync_id_;
  if (!prefs_->GetBoolean(prefs::kBraveAIChatUserMemoryEnabled)) {
    memory_sync_pending_ = false;
    db_.AsyncCall(
        base::IgnoreResult(&AIChatEmbeddingsDatabase::DeleteAllMemories));
    return;
  }
  memory_sync_pending_ = true;
  db_.AsyncCall(&AIChatEmbeddingsDatabase::SyncMemories)
      .WithArgs(prefs::GetMemoriesFromPrefs(*prefs_))
      .Then(base::BindOnce(&AIChatEmbeddingsService::OnMemoriesSynced,
                           indexing_weak_ptr_factory_.GetWeakPtr(),
                           memory_sync_id_));
}

void AIChatEmbeddingsService::OnMemoriesSynced(
    uint64_t sync_id,
    std::vector<std::string> missing_memories) {
  // Memories missing at an earlier sync can have been stored since.
  if (sync_id != memory_sync_id_) {
    return;
  }
  memory_sync_pending_ = false;
  if (missing_memories.empty()) {
    return;
  }
  std::vector<std::string> documents = missing_memories;
  memory_job_ = embedder_->ComputePassagesEmbeddings(
      PassagePriority::kPassive, std::move(documents),
      base::BindOnce(&AIChatEmbeddingsService::OnMemoriesEmbedded,
                     indexing_weak_ptr_factory_.GetWeakPtr(),
                     std::move(missing_memories)));
}

void AIChatEmbeddingsService::OnMemoriesEmbedded(
    std::vector<std::string> memories,
    std::vector<std::string> documents,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    ComputeEmbeddingsStatus status) {
  if (!memory_job_ || memory_job_->id() != job_id) {
    return;
  }
  memory_job_.reset();
  if (status != ComputeEmbeddingsStatus::kSuccess ||
      embeddings.size() != memories.size()) {
    return;
  }
  // Leaves out memories deleted while they were embedded.
  const std::vector<std::string> current_memories =
      prefs::GetMemoriesFromPrefs(*prefs_);
  std::vector<MemoryPassage> passages;
  for (size_t i = 0; i < memories.size(); ++i) {
    if (std::ranges::contains(current_memories, memories[i])) {
      passages.push_back({std::move(memories[i]), embeddings[i].GetData()});
    }
  }
  db_.AsyncCall(base::IgnoreResult(&AIChatEmbeddingsDatabase::AddMemories))
      .WithArgs(std::move(passages));
}

void AIChatEmbeddingsService::EmbedQuery(const std::string& query,
                                         EmbeddingCallback callback) {
  if (!db_ || query.empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  query_jobs_.push_back(embedder_->ComputePassagesEmbeddings(
      PassagePriority::kUserInitiated, {query},
      base::BindOnce(&AIChatEmbeddingsService::OnQueryEmbedded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void AIChatEmbeddingsService::OnQueryEmbedded(
    EmbeddingCallback callback,
    std::vector<std::string> queries,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    ComputeEmbeddingsStatus status) {
  std::erase_if(query_jobs_,
                [job_id](const passage_embeddings::Embedder::Job& job) {
                  return job.id() == job_id;
                });
  if (status != ComputeEmbeddingsStatus::kSuccess || embeddings.size() != 1) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(std::move(embeddings[0]));
}

void AIChatEmbeddingsService::OnConversationQueryEmbedded(
    size_t count,
    const std::string& excluded_conversation_uuid,
    SearchConversationsCallback callback,
    std::optional<passage_embeddings::Embedding> query) {
  if (!query || !db_) {
    std::move(callback).Run({});
    return;
  }
  db_.AsyncCall(&AIChatEmbeddingsDatabase::SearchConversations)
      .WithArgs(std::move(*query), GetMinScore(), count,
                kMaxPassagesPerConversation, excluded_conversation_uuid)
      .Then(base::BindOnce(&AIChatEmbeddingsService::OnConversationsFound,
                           weak_ptr_factory_.GetWeakPtr(),
                           std::move(callback)));
}

void AIChatEmbeddingsService::OnConversationsFound(
    SearchConversationsCallback callback,
    std::vector<ConversationMatch> matches) {
  if (matches.empty()) {
    std::move(callback).Run({});
    return;
  }
  ai_chat_service_->GetConversations(base::BindOnce(
      &AIChatEmbeddingsService::OnGotConversationsForResults,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback), std::move(matches)));
}

void AIChatEmbeddingsService::OnGotConversationsForResults(
    SearchConversationsCallback callback,
    std::vector<ConversationMatch> matches,
    std::vector<mojom::ConversationPtr> conversations) {
  const auto conversations_by_uuid =
      base::MakeFlatMap<std::string_view, const mojom::Conversation*>(
          conversations, {}, [](const mojom::ConversationPtr& conversation) {
            return std::make_pair(std::string_view(conversation->uuid),
                                  conversation.get());
          });
  std::vector<ConversationSearchResult> results;
  for (ConversationMatch& match : matches) {
    auto it = conversations_by_uuid.find(match.conversation_uuid);
    // Deleted since it was indexed.
    if (it == conversations_by_uuid.end() || it->second->temporary) {
      continue;
    }
    ConversationSearchResult result;
    result.conversation_uuid = std::move(match.conversation_uuid);
    result.title = it->second->title;
    result.updated_time = it->second->updated_time;
    for (ConversationPassageMatch& passage : match.passages) {
      // The title is given on its own.
      if (!passage.entry_uuid.empty()) {
        result.passages.push_back(std::move(passage));
      }
    }
    results.push_back(std::move(result));
  }
  std::move(callback).Run(std::move(results));
}

void AIChatEmbeddingsService::OnMemoryQueryEmbedded(
    size_t count,
    SearchMemoriesCallback callback,
    std::optional<passage_embeddings::Embedding> query) {
  if (!query || !db_) {
    std::move(callback).Run({});
    return;
  }
  db_.AsyncCall(&AIChatEmbeddingsDatabase::SearchMemories)
      .WithArgs(std::move(*query), GetMinScore(), count)
      .Then(base::BindOnce(
          [](SearchMemoriesCallback callback,
             std::vector<MemoryMatch> matches) {
            std::move(callback).Run(
                base::ToVector(matches, &MemoryMatch::text));
          },
          std::move(callback)));
}

float AIChatEmbeddingsService::GetMinScore() const {
  if (embedder_metadata_ && embedder_metadata_->search_score_threshold) {
    return static_cast<float>(*embedder_metadata_->search_score_threshold);
  }
  return kDefaultMinScore;
}

}  // namespace ai_chat
