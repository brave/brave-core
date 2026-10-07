// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_AI_CHAT_EMBEDDINGS_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_AI_CHAT_EMBEDDINGS_SERVICE_H_

#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_database.h"
#include "brave/components/ai_chat/core/browser/learned_memory_search.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

namespace ai_chat {

// A stored conversation found by
// AIChatEmbeddingsService::SearchConversations().
struct ConversationSearchResult {
  ConversationSearchResult();
  ConversationSearchResult(ConversationSearchResult&&);
  ConversationSearchResult& operator=(ConversationSearchResult&&);
  ~ConversationSearchResult();

  std::string conversation_uuid;
  std::string title;
  base::Time updated_time;
  // The matching passages of the conversation's entries, best first. Empty
  // when only the title matched.
  std::vector<ConversationPassageMatch> passages;
};

// A memory found by AIChatEmbeddingsService::SearchMemories(): one that the
// user wrote, or one that Leo learned from the saved chats.
struct MemorySearchResult {
  std::string text;
  // The uuid of a learned memory. Empty for a memory that the user wrote.
  std::string learned_uuid;
  float score = 0.0f;
};

// Keeps an on-device index of embeddings of the user's stored Leo
// conversations and memories, and searches it semantically. Entries are indexed
// as they are persisted, and whenever storage becomes ready the index is
// reconciled with the stored conversations, which picks up changes made while
// the service wasn't running. Turning semantic search off deletes the index.
//
// Learned memories are synced with the conversation database as a whole: each
// change that can add, edit or delete one compares the stored text versions
// with the index, deletes stale embeddings and embeds what is missing.
class AIChatEmbeddingsService
    : public KeyedService,
      public AIChatService::Observer,
      public passage_embeddings::EmbedderMetadataObserver,
      public LearnedMemorySearch {
 public:
  using SearchConversationsCallback =
      base::OnceCallback<void(std::vector<ConversationSearchResult>)>;
  using SearchMemoriesCallback =
      base::OnceCallback<void(std::vector<MemorySearchResult>)>;

  // `embedder` and `embedder_metadata_provider` must outlive the service.
  AIChatEmbeddingsService(
      AIChatService* ai_chat_service,
      PrefService* prefs,
      os_crypt_async::OSCryptAsync* os_crypt_async,
      passage_embeddings::Embedder* embedder,
      passage_embeddings::EmbedderMetadataProvider* embedder_metadata_provider,
      const base::FilePath& profile_path);
  AIChatEmbeddingsService(const AIChatEmbeddingsService&) = delete;
  AIChatEmbeddingsService& operator=(const AIChatEmbeddingsService&) = delete;
  ~AIChatEmbeddingsService() override;

  // Deletes the index of the profile at `profile_path`, for a profile whose
  // service isn't running.
  static void DeleteIndex(const base::FilePath& profile_path);

  // Finds up to `count` stored conversations related to `query`, other than
  // the one with `excluded_conversation_uuid`, best first. Finds nothing until
  // the index is ready.
  void SearchConversations(const std::string& query,
                           size_t count,
                           const std::string& excluded_conversation_uuid,
                           SearchConversationsCallback callback);

  // Finds up to `count` memories related to `query`, best first: the ones that
  // the user wrote, and the learned ones while learned memory is on. Both must
  // score at least the score threshold of the model. Finds nothing while
  // memories are turned off.
  void SearchMemories(const std::string& query,
                      size_t count,
                      SearchMemoriesCallback callback);

  // LearnedMemorySearch:
  bool IsLearnedMemoryIndexCurrent() const override;
  void WhenLearnedMemoryIndexCurrent(base::OnceClosure callback) override;
  void SearchLearnedMemories(std::vector<std::string> queries,
                             size_t count,
                             SearchCallback callback) override;
  void SearchLearnedMemoriesByEmbedding(std::vector<float> embedding,
                                        size_t count,
                                        SearchCallback callback) override;

  base::WeakPtr<AIChatEmbeddingsService> GetWeakPtr();

  // Runs `callback` once every change handed to the index so far is stored.
  void FlushForTesting(base::OnceClosure callback);

  // Whether indexing is neither in progress nor due.
  bool IsIndexingIdleForTesting() const;

  // KeyedService:
  void Shutdown() override;

 private:
  using EmbeddingCallback =
      base::OnceCallback<void(std::optional<passage_embeddings::Embedding>)>;

  // AIChatService::Observer:
  void OnStorageReady() override;
  void OnConversationEntryAdded(const std::string& conversation_uuid,
                                const mojom::ConversationTurn& entry) override;
  void OnConversationEntryRemoved(const std::string& conversation_uuid,
                                  const std::string& entry_uuid) override;
  void OnConversationTitleChanged(const std::string& conversation_uuid,
                                  const std::string& title) override;
  void OnConversationDeleted(const std::string& conversation_uuid) override;
  void OnAllConversationsDeleted() override;
  void OnLearnedMemoriesChanged() override;

  // passage_embeddings::EmbedderMetadataObserver:
  void EmbedderMetadataUpdated(
      passage_embeddings::EmbedderMetadata metadata) override;

  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);
  void MaybeCreateDatabase();
  // Drops all indexing in progress.
  void ResetIndexing();
  void OnSemanticSearchPrefChanged();
  void OnUserMemoryEnabledPrefChanged();

  void MaybeReconcileConversations();
  void OnGotIndexedConversations(
      base::flat_map<std::string, base::Time> indexed_conversations);
  void ReconcileConversations(
      base::flat_map<std::string, base::Time> indexed_conversations,
      std::vector<mojom::ConversationPtr> conversations);

  // Leaves changes to a conversation to the full indexing it is queued for,
  // which reads the conversation afresh. Notes that a conversation being
  // indexed has to be indexed again.
  bool DeferToConversationIndexing(const std::string& conversation_uuid);
  void QueueConversation(const std::string& conversation_uuid);
  void MaybeIndexNextConversation();
  void OnGotConversationsForIndexing(
      const std::string& conversation_uuid,
      std::vector<mojom::ConversationPtr> conversations);
  void OnGotConversationEntries(
      const std::string& conversation_uuid,
      const std::string& title,
      base::Time updated_time,
      std::vector<mojom::ConversationTurnPtr> entries);
  void OnConversationEmbedded(
      const std::string& conversation_uuid,
      base::Time updated_time,
      std::vector<ConversationPassage> passages,
      std::vector<std::string> documents,
      std::vector<passage_embeddings::Embedding> embeddings,
      uint64_t job_id,
      passage_embeddings::ComputeEmbeddingsStatus status);
  void FinishConversationIndexing();

  // Indexes `passages` of the entry with `entry_uuid`, which is empty for the
  // conversation's title.
  void IndexEntryPassages(const std::string& conversation_uuid,
                          const std::string& entry_uuid,
                          std::vector<ConversationPassage> passages,
                          std::optional<base::Time> indexed_time);
  void OnEntryEmbedded(const std::string& conversation_uuid,
                       const std::string& entry_uuid,
                       std::optional<base::Time> indexed_time,
                       std::vector<ConversationPassage> passages,
                       std::vector<std::string> documents,
                       std::vector<passage_embeddings::Embedding> embeddings,
                       uint64_t job_id,
                       passage_embeddings::ComputeEmbeddingsStatus status);
  void CancelEntryIndexing(const std::string& conversation_uuid);

  void SyncMemories();
  void OnMemoriesSynced(uint64_t sync_id,
                        std::vector<std::string> missing_memories);
  void OnMemoriesEmbedded(std::vector<std::string> memories,
                          std::vector<std::string> documents,
                          std::vector<passage_embeddings::Embedding> embeddings,
                          uint64_t job_id,
                          passage_embeddings::ComputeEmbeddingsStatus status);

  void SyncLearnedMemories();
  void OnGotLearnedMemoryStamps(uint64_t sync_id,
                                std::vector<LearnedMemoryStamp> stamps);
  void OnLearnedMemoriesSynced(uint64_t sync_id,
                               std::vector<std::string> missing_uuids);
  void OnGotLearnedMemoriesToEmbed(uint64_t sync_id,
                                   std::vector<LearnedMemory> memories);
  void OnLearnedMemoriesEmbedded(
      uint64_t sync_id,
      std::vector<LearnedMemoryStamp> stamps,
      std::vector<std::string> documents,
      std::vector<passage_embeddings::Embedding> embeddings,
      uint64_t job_id,
      passage_embeddings::ComputeEmbeddingsStatus status);
  // The sync with `sync_id` ended. Unless a later sync started, the index is
  // current.
  void OnLearnedMemorySyncDone(uint64_t sync_id);
  void OnLearnedMemoryQueriesEmbedded(
      size_t count,
      SearchCallback callback,
      std::vector<std::string> queries,
      std::vector<passage_embeddings::Embedding> embeddings,
      uint64_t job_id,
      passage_embeddings::ComputeEmbeddingsStatus status);

  void EmbedQuery(const std::string& query, EmbeddingCallback callback);
  void OnQueryEmbedded(EmbeddingCallback callback,
                       std::vector<std::string> queries,
                       std::vector<passage_embeddings::Embedding> embeddings,
                       uint64_t job_id,
                       passage_embeddings::ComputeEmbeddingsStatus status);
  void OnConversationQueryEmbedded(
      size_t count,
      const std::string& excluded_conversation_uuid,
      SearchConversationsCallback callback,
      std::optional<passage_embeddings::Embedding> query);
  void OnConversationsFound(SearchConversationsCallback callback,
                            std::vector<ConversationMatch> matches);
  void OnGotConversationsForResults(
      SearchConversationsCallback callback,
      std::vector<ConversationMatch> matches,
      std::vector<mojom::ConversationPtr> conversations);
  void OnMemoryQueryEmbedded(
      size_t count,
      SearchMemoriesCallback callback,
      std::optional<passage_embeddings::Embedding> query);
  void OnUserMemoriesFound(size_t count,
                           float min_score,
                           std::vector<float> query,
                           SearchMemoriesCallback callback,
                           std::vector<MemoryMatch> matches);
  void OnLearnedMemoriesFound(size_t count,
                              std::vector<MemorySearchResult> results,
                              SearchMemoriesCallback callback,
                              std::vector<LearnedMemoryMatch> matches);
  void OnGotLearnedMemoriesForResults(size_t count,
                                      std::vector<MemorySearchResult> results,
                                      SearchMemoriesCallback callback,
                                      base::flat_map<std::string, float> scores,
                                      std::vector<LearnedMemory> memories);
  float GetMinScore() const;

  const base::FilePath db_file_path_;
  const scoped_refptr<base::SequencedTaskRunner> db_task_runner_;

  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  std::optional<passage_embeddings::EmbedderMetadata> embedder_metadata_;
  base::SequenceBound<AIChatEmbeddingsDatabase> db_;
  // Set once semantic search is turned off, after which the service does
  // nothing.
  bool disabled_ = false;

  // Reconciliations waiting for the index or for the conversations.
  int reconciliations_in_progress_ = 0;

  // Conversations waiting to be indexed in full, and the one being indexed.
  std::deque<std::string> conversations_to_index_;
  std::optional<std::string> indexing_conversation_uuid_;
  bool index_conversation_again_ = false;
  std::optional<passage_embeddings::Embedder::Job> conversation_job_;

  // Entries being embedded, keyed by conversation uuid and entry uuid.
  std::map<std::pair<std::string, std::string>,
           passage_embeddings::Embedder::Job>
      entry_jobs_;
  std::optional<passage_embeddings::Embedder::Job> memory_job_;
  // Identifies the latest request to sync memories, whose result is awaited
  // while `memory_sync_pending_`. An earlier request's result is stale.
  uint64_t memory_sync_id_ = 0;
  bool memory_sync_pending_ = false;
  // Identifies the latest sync of learned memories. A new sync drops the work
  // of the earlier one, so that a memory deleted meanwhile is not embedded.
  uint64_t learned_memory_sync_id_ = 0;
  bool learned_memory_index_current_ = false;
  std::optional<passage_embeddings::Embedder::Job> learned_memory_job_;
  std::vector<base::OnceClosure> learned_memory_index_waiters_;
  std::vector<passage_embeddings::Embedder::Job> query_jobs_;

  PrefChangeRegistrar pref_change_registrar_;
  base::ScopedObservation<AIChatService, AIChatService::Observer>
      ai_chat_service_observation_{this};
  base::ScopedObservation<passage_embeddings::EmbedderMetadataProvider,
                          passage_embeddings::EmbedderMetadataObserver>
      embedder_metadata_observation_{this};

  const raw_ref<AIChatService> ai_chat_service_;
  const raw_ref<PrefService> prefs_;
  const raw_ref<passage_embeddings::Embedder> embedder_;

  // For the callbacks of indexing, which ResetIndexing() drops.
  base::WeakPtrFactory<AIChatEmbeddingsService> indexing_weak_ptr_factory_{
      this};
  base::WeakPtrFactory<AIChatEmbeddingsService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_AI_CHAT_EMBEDDINGS_SERVICE_H_
