// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_database.h"
#include "brave/components/ai_chat/core/browser/embeddings/fake_embedder.h"
#include "brave/components/ai_chat/core/browser/learned_memory_search.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

using passage_embeddings::PassagePriority;
using testing::Contains;
using testing::ElementsAre;
using testing::Pair;
using testing::UnorderedElementsAre;

namespace {

// Reports no premium, without reaching for a SKU service.
class FakeAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;

  void GetPremiumStatus(
      mojom::Service::GetPremiumStatusCallback callback) override {
    std::move(callback).Run(mojom::PremiumStatus::Inactive,
                            mojom::PremiumInfo::New());
  }
};

}  // namespace

class AIChatEmbeddingsServiceTest : public testing::Test {
 public:
  AIChatEmbeddingsServiceTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kAIChatHistory);
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
    local_ai::prefs::RegisterProfilePrefs(prefs_.registry());
    prefs_.SetBoolean(local_ai::prefs::kBraveHistoryEmbeddingsEnabled, true);
    prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, true);
    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    model_service_ = std::make_unique<ModelService>(
        &prefs_, os_crypt_.get(), network::NetworkContextGetter(),
        /*url_loader_factory=*/nullptr, base::FilePath());
    tab_tracker_service_ = std::make_unique<TabTrackerService>();
    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), tab_tracker_service_.get(),
        std::make_unique<FakeAIChatCredentialManager>(base::NullCallback(),
                                                      &local_state_),
        &prefs_, /*ai_chat_metrics=*/nullptr, os_crypt_.get(),
        shared_url_loader_factory_, "", temp_dir_.GetPath());
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return ai_chat_service_->IsStorageReady(); }));
    CreateService();
  }

  void TearDown() override {
    DestroyService();
    ai_chat_service_.reset();
    // Lets the conversation database close before its directory is deleted.
    base::ThreadPoolInstance::Get()->FlushForTesting();
  }

 protected:
  void CreateService() {
    service_ = std::make_unique<AIChatEmbeddingsService>(
        ai_chat_service_.get(), &prefs_, os_crypt_.get(), &embedder_,
        &embedder_metadata_provider_, temp_dir_.GetPath());
  }

  void DestroyService() {
    if (!service_) {
      return;
    }
    service_->Shutdown();
    // Lets the index close before the service goes.
    Flush();
    service_.reset();
  }

  void Flush() {
    base::test::TestFuture<void> future;
    service_->FlushForTesting(future.GetCallback());
    ASSERT_TRUE(future.Wait());
  }

  // Waits for a passage containing `text` to be embedded, and for the index to
  // store it and settle.
  void WaitForIndexed(std::string_view text) {
    ASSERT_TRUE(base::test::RunUntil([&] {
      return embedder_.HasEmbedded(text) &&
             service_->IsIndexingIdleForTesting();
    }));
    Flush();
  }

  size_t CountEmbedded(std::string_view text) const {
    return std::ranges::count_if(
        embedder_.embedded_passages(), [&](const auto& passage) {
          return passage.first.find(text) != std::string::npos;
        });
  }

  // Persists a conversation of one query and its response.
  ConversationHandler* AddConversation(const std::string& query,
                                       const std::string& response) {
    ConversationHandler* conversation = ai_chat_service_->CreateConversation();
    conversation->SetChatHistoryForTesting(MakeHistory(query, response));
    return conversation;
  }

  static std::vector<mojom::ConversationTurnPtr> MakeHistory(
      const std::string& query,
      const std::string& response) {
    std::vector<mojom::ConversationTurnPtr> history =
        CreateSampleChatHistory(1u);
    history[0]->text = query;
    history[1]->events->clear();
    history[1]->events->push_back(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(response)));
    return history;
  }

  std::vector<ConversationSearchResult> SearchConversations(
      const std::string& query,
      const std::string& excluded_conversation_uuid = "") {
    base::test::TestFuture<std::vector<ConversationSearchResult>> future;
    service_->SearchConversations(
        query, /*count=*/10, excluded_conversation_uuid, future.GetCallback());
    return future.Take();
  }

  std::vector<MemorySearchResult> SearchMemoryResults(const std::string& query,
                                                      size_t count = 10) {
    base::test::TestFuture<std::vector<MemorySearchResult>> future;
    service_->SearchMemories(query, count, future.GetCallback());
    return future.Take();
  }

  // The texts of the memories found, the learned ones included.
  std::vector<std::string> SearchMemories(const std::string& query) {
    return base::ToVector(SearchMemoryResults(query),
                          &MemorySearchResult::text);
  }

  // Writes a learned memory through AIChatService, the same way Dreaming does.
  void WriteLearnedMemory(const std::string& uuid,
                          const std::string& text,
                          std::vector<MemorySourceLink> links = {}) {
    LearnedMemory memory;
    memory.uuid = uuid;
    memory.text = text;
    memory.links = std::move(links);
    base::test::TestFuture<bool> future;
    ai_chat_service_->AddOrUpdateLearnedMemory(std::move(memory),
                                               future.GetCallback());
    ASSERT_TRUE(future.Get());
  }

  void DeleteLearnedMemory(const std::string& uuid) {
    base::test::TestFuture<bool> future;
    ai_chat_service_->DeleteLearnedMemory(uuid, future.GetCallback());
    ASSERT_TRUE(future.Get());
  }

  // Waits until the learned memories are synced, and the index settled.
  void WaitForLearnedMemoriesSynced() {
    ASSERT_TRUE(base::test::RunUntil([&] {
      return service_->IsLearnedMemoryIndexCurrent() &&
             service_->IsIndexingIdleForTesting();
    }));
    Flush();
  }

  // The uuids of the learned memories that are close to `query`.
  std::vector<std::string> FindLearnedMemories(const std::string& query) {
    base::test::TestFuture<std::vector<LearnedMemoryMatch>> future;
    service_->SearchLearnedMemories({query}, /*count=*/10,
                                    future.GetCallback());
    std::vector<std::string> uuids;
    for (const LearnedMemoryMatch& match : future.Take()) {
      if (match.score > 0.5f) {
        uuids.push_back(match.uuid);
      }
    }
    return uuids;
  }

  static std::vector<std::string> GetPassageTexts(
      const ConversationSearchResult& result) {
    return base::ToVector(result.passages, &ConversationPassageMatch::text);
  }

  base::FilePath db_file_path() const {
    return temp_dir_.GetPath().Append(kAIChatEmbeddingsDatabaseFileName);
  }

  // Declared first so that the directory outlives everything using it.
  base::ScopedTempDir temp_dir_;
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<TabTrackerService> tab_tracker_service_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  FakeEmbedder embedder_;
  FakeEmbedderMetadataProvider embedder_metadata_provider_;
  std::unique_ptr<AIChatEmbeddingsService> service_;
};

TEST_F(AIChatEmbeddingsServiceTest, IndexesPersistedEntries) {
  const std::string uuid =
      AddConversation("Tell me about my cat", "Cats sleep for most of the day.")
          ->get_conversation_uuid();
  WaitForIndexed("Cats sleep for most of the day.");

  std::vector<ConversationSearchResult> results = SearchConversations("cat");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].conversation_uuid, uuid);
  EXPECT_THAT(GetPassageTexts(results[0]),
              UnorderedElementsAre("Tell me about my cat",
                                   "Cats sleep for most of the day."));
  EXPECT_TRUE(SearchConversations("bird").empty());
  // The conversation the search is made from is left out.
  EXPECT_TRUE(SearchConversations("cat", uuid).empty());
}

TEST_F(AIChatEmbeddingsServiceTest, IndexesTitles) {
  ConversationHandler* conversation =
      AddConversation("Tell me something", "Here is what I found for you.");
  const std::string uuid = conversation->get_conversation_uuid();
  ai_chat_service_->OnConversationTitleChanged(uuid,
                                               "Bird watching in the park");
  WaitForIndexed("Bird watching in the park");

  std::vector<ConversationSearchResult> results = SearchConversations("bird");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].conversation_uuid, uuid);
  EXPECT_EQ(results[0].title, "Bird watching in the park");
  // The title is given on its own rather than as a passage.
  EXPECT_TRUE(results[0].passages.empty());
}

TEST_F(AIChatEmbeddingsServiceTest, EmbedsTextAsItIsWithPriorities) {
  AddConversation("Tell me about my cat", "Cats sleep for most of the day.");
  WaitForIndexed("Cats sleep for most of the day.");
  SearchConversations("cat");

  EXPECT_THAT(embedder_.embedded_passages(),
              Contains(Pair("Cats sleep for most of the day.",
                            PassagePriority::kPassive)));
  EXPECT_THAT(embedder_.embedded_passages(),
              Contains(Pair("cat", PassagePriority::kUserInitiated)));
}

TEST_F(AIChatEmbeddingsServiceTest, LeavesOutPassagesTooShortToScore) {
  AddConversation("Thanks a lot!", "Cats sleep for most of the day.");
  // Words can't be counted in a script written without spaces.
  AddConversation("猫", "Dogs bark at the mail carrier.");
  WaitForIndexed("Cats sleep for most of the day.");
  WaitForIndexed("Dogs bark at the mail carrier.");

  EXPECT_FALSE(embedder_.HasEmbedded("Thanks a lot!"));
  EXPECT_TRUE(embedder_.HasEmbedded("猫"));
}

TEST_F(AIChatEmbeddingsServiceTest, SplitsEntriesIntoPassagesOf100Words) {
  auto words = [](size_t count) {
    return base::JoinString(std::vector<std::string>(count, "cat"), " ");
  };
  AddConversation("Tell me about my dog", words(150));
  WaitForIndexed("cat cat");

  EXPECT_THAT(embedder_.embedded_passages(),
              Contains(Pair(words(100), PassagePriority::kPassive)));
  EXPECT_THAT(embedder_.embedded_passages(),
              Contains(Pair(words(50), PassagePriority::kPassive)));
  EXPECT_FALSE(embedder_.HasEmbedded(words(101)));
}

TEST_F(AIChatEmbeddingsServiceTest, IgnoresTemporaryConversations) {
  ConversationHandler* temporary_conversation =
      ai_chat_service_->CreateConversation();
  temporary_conversation->SetTemporary(true);
  temporary_conversation->SetChatHistoryForTesting(
      MakeHistory("Tell me about my cat", "Cats sleep for most of the day."));
  AddConversation("Tell me about my dog", "Dogs bark at the mail carrier.");
  WaitForIndexed("Dogs bark at the mail carrier.");

  EXPECT_FALSE(embedder_.HasEmbedded("Cats sleep for most of the day."));
  EXPECT_TRUE(SearchConversations("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, IndexesTheLatestEdit) {
  ConversationHandler* conversation =
      AddConversation("Tell me something", "Cats are great company at home.");
  // The response is edited before its first version is indexed.
  mojom::ConversationTurnPtr response =
      conversation->GetConversationHistory()[1]->Clone();
  mojom::ConversationTurnPtr edit = response->Clone();
  edit->uuid = "edit";
  edit->events->clear();
  edit->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Dogs are loyal to their owners.")));
  response->edits.emplace();
  response->edits->push_back(std::move(edit));
  ai_chat_service_->OnConversationEntryRemoved(conversation, *response->uuid);
  ai_chat_service_->OnConversationEntryAdded(conversation, response,
                                             std::nullopt);
  WaitForIndexed("Dogs are loyal to their owners.");

  EXPECT_TRUE(SearchConversations("cat").empty());
  std::vector<ConversationSearchResult> results = SearchConversations("dog");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_THAT(GetPassageTexts(results[0]),
              ElementsAre("Dogs are loyal to their owners."));
}

TEST_F(AIChatEmbeddingsServiceTest, RemovesDeletedEntries) {
  ConversationHandler* conversation =
      AddConversation("Tell me something", "Cats are great company at home.");
  const std::string uuid = conversation->get_conversation_uuid();
  const std::string response_uuid =
      *conversation->GetConversationHistory()[1]->uuid;
  WaitForIndexed("Cats are great company at home.");
  ASSERT_EQ(SearchConversations("cat").size(), 1u);

  conversation = ai_chat_service_->GetConversation(uuid);
  ASSERT_TRUE(conversation);
  ai_chat_service_->OnConversationEntryRemoved(conversation, response_uuid);
  Flush();
  EXPECT_TRUE(SearchConversations("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, RemovesDeletedConversations) {
  const std::string cat_uuid =
      AddConversation("Tell me about my cat", "Cats sleep for most of the day.")
          ->get_conversation_uuid();
  AddConversation("Tell me about my dog", "Dogs bark at the mail carrier.");
  WaitForIndexed("Cats sleep for most of the day.");
  WaitForIndexed("Dogs bark at the mail carrier.");

  ai_chat_service_->DeleteConversation(cat_uuid);
  Flush();
  EXPECT_TRUE(SearchConversations("cat").empty());
  EXPECT_EQ(SearchConversations("dog").size(), 1u);

  ai_chat_service_->DeleteConversations();
  Flush();
  EXPECT_TRUE(SearchConversations("dog").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, RemovesConversationsWhenStorageIsOff) {
  AddConversation("Tell me about my cat", "Cats sleep for most of the day.");
  WaitForIndexed("Cats sleep for most of the day.");

  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);
  Flush();
  EXPECT_TRUE(SearchConversations("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, IndexesConversationsStoredWhileNotRunning) {
  DestroyService();
  const std::string uuid =
      AddConversation("Tell me about my cat", "Cats sleep for most of the day.")
          ->get_conversation_uuid();
  CreateService();
  WaitForIndexed("Cats sleep for most of the day.");

  std::vector<ConversationSearchResult> results = SearchConversations("cat");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].conversation_uuid, uuid);
}

TEST_F(AIChatEmbeddingsServiceTest,
       RemovesConversationsDeletedWhileNotRunning) {
  const std::string cat_uuid =
      AddConversation("Tell me about my cat", "Cats sleep for most of the day.")
          ->get_conversation_uuid();
  WaitForIndexed("Cats sleep for most of the day.");
  DestroyService();
  ai_chat_service_->DeleteConversation(cat_uuid);
  // Its indexing marks the end of the reconciliation.
  AddConversation("Tell me about my dog", "Dogs bark at the mail carrier.");

  CreateService();
  WaitForIndexed("Dogs bark at the mail carrier.");
  EXPECT_TRUE(SearchConversations("cat").empty());
  EXPECT_EQ(SearchConversations("dog").size(), 1u);
}

TEST_F(AIChatEmbeddingsServiceTest, IndexesAgainForNewModel) {
  AddConversation("Tell me about my cat", "Cats sleep for most of the day.");
  WaitForIndexed("Cats sleep for most of the day.");
  const size_t embedded_count =
      CountEmbedded("Cats sleep for most of the day.");

  embedder_metadata_provider_.SetMetadata(passage_embeddings::EmbedderMetadata(
      /*model_version=*/2, /*output_size=*/4, /*search_score_threshold=*/0.5));
  ASSERT_TRUE(base::test::RunUntil([&] {
    return CountEmbedded("Cats sleep for most of the day.") > embedded_count &&
           service_->IsIndexingIdleForTesting();
  }));
  Flush();
  EXPECT_EQ(SearchConversations("cat").size(), 1u);
}

TEST_F(AIChatEmbeddingsServiceTest, IndexesMemories) {
  prefs::AddMemoryToPrefs("Has a cat named Tom", prefs_);
  prefs::AddMemoryToPrefs("Walks the dog daily", prefs_);
  WaitForIndexed("Walks the dog daily");
  ASSERT_TRUE(embedder_.HasEmbedded("Has a cat named Tom"));

  EXPECT_THAT(SearchMemories("dog"), ElementsAre("Walks the dog daily"));
  EXPECT_THAT(SearchMemories("cat"), ElementsAre("Has a cat named Tom"));

  prefs::DeleteMemoryFromPrefs("Walks the dog daily", prefs_);
  Flush();
  EXPECT_TRUE(SearchMemories("dog").empty());

  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);
  EXPECT_TRUE(SearchMemories("cat").empty());
}

// One query finds the memories that the user wrote and the learned ones, and
// only a learned memory has a uuid.
TEST_F(AIChatEmbeddingsServiceTest, SearchesMemoriesAndLearnedMemories) {
  base::test::ScopedFeatureList learned_memory_feature;
  learned_memory_feature.InitAndEnableFeature(features::kAIChatLearnedMemory);
  prefs::AddMemoryToPrefs("Walks the dog daily", prefs_);
  prefs::AddMemoryToPrefs("Has a cat named Tom", prefs_);
  WriteLearnedMemory("dog-memory", "Has two dogs");
  WriteLearnedMemory("bird-memory", "Feeds the birds");
  WaitForIndexed("Walks the dog daily");
  WaitForLearnedMemoriesSynced();

  std::vector<MemorySearchResult> results = SearchMemoryResults("dog");
  EXPECT_THAT(base::ToVector(results,
                             [](const MemorySearchResult& result) {
                               return std::make_pair(result.text,
                                                     result.learned_uuid);
                             }),
              UnorderedElementsAre(Pair("Walks the dog daily", ""),
                                   Pair("Has two dogs", "dog-memory")));
  for (const MemorySearchResult& result : results) {
    EXPECT_GT(result.score, 0.5f);
  }

  // Only a learned memory is related to this query.
  EXPECT_THAT(SearchMemories("bird"), ElementsAre("Feeds the birds"));
  EXPECT_TRUE(SearchMemories("fish").empty());

  // The best ones fit in `count`, whether the user wrote them or not.
  EXPECT_EQ(SearchMemoryResults("dog", /*count=*/1).size(), 1u);

  // A deleted memory is not found, even before the index drops its embedding.
  DeleteLearnedMemory("dog-memory");
  EXPECT_THAT(SearchMemories("dog"), ElementsAre("Walks the dog daily"));
}

TEST_F(AIChatEmbeddingsServiceTest, SearchesNoLearnedMemoriesWhileTheyAreOff) {
  prefs::AddMemoryToPrefs("Walks the dog daily", prefs_);
  WriteLearnedMemory("dog-memory", "Has two dogs");
  WaitForIndexed("Walks the dog daily");
  WaitForLearnedMemoriesSynced();

  // Learned memory is off: the learned memories stay in the database, and the
  // search leaves them out.
  EXPECT_THAT(SearchMemories("dog"), ElementsAre("Walks the dog daily"));

  base::test::ScopedFeatureList learned_memory_feature;
  learned_memory_feature.InitAndEnableFeature(features::kAIChatLearnedMemory);
  EXPECT_THAT(SearchMemories("dog"),
              UnorderedElementsAre("Walks the dog daily", "Has two dogs"));

  // Turning memories off turns off both.
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);
  EXPECT_TRUE(SearchMemories("dog").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, IndexesLearnedMemories) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  WriteLearnedMemory("dog-memory", "Walks the dog daily");
  WaitForLearnedMemoriesSynced();

  EXPECT_THAT(FindLearnedMemories("dog"), ElementsAre("dog-memory"));
  EXPECT_THAT(FindLearnedMemories("cat"), ElementsAre("cat-memory"));
  EXPECT_THAT(embedder_.embedded_passages(),
              Contains(Pair("Has a cat named Tom", PassagePriority::kPassive)));
  EXPECT_THAT(embedder_.embedded_passages(),
              Contains(Pair("dog", PassagePriority::kUserInitiated)));

  // Dreaming searches with the embedding it has: a unit vector along "cat".
  base::test::TestFuture<std::vector<LearnedMemoryMatch>> future;
  service_->SearchLearnedMemoriesByEmbedding({1.0f, 0.0f, 0.0f, 0.0f},
                                             /*count=*/1, future.GetCallback());
  std::vector<LearnedMemoryMatch> matches = future.Take();
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].uuid, "cat-memory");

  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, EmbedsLearnedMemoryAgainWhenTextChanges) {
  WriteLearnedMemory("memory", "Has a cat named Tom");
  WaitForLearnedMemoriesSynced();
  ASSERT_THAT(FindLearnedMemories("cat"), ElementsAre("memory"));

  // A replace or a merge: the same memory gets a new text.
  WriteLearnedMemory("memory", "Feeds the birds");
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
  EXPECT_THAT(FindLearnedMemories("bird"), ElementsAre("memory"));

  // A new mention of the same fact keeps the text, so nothing is embedded.
  WriteLearnedMemory("memory", "Feeds the birds");
  WaitForLearnedMemoriesSynced();
  EXPECT_EQ(CountEmbedded("Feeds the birds"), 1u);
}

TEST_F(AIChatEmbeddingsServiceTest, RemovesDeletedLearnedMemories) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  WriteLearnedMemory("dog-memory", "Walks the dog daily");
  WaitForLearnedMemoriesSynced();

  DeleteLearnedMemory("cat-memory");
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
  EXPECT_THAT(FindLearnedMemories("dog"), ElementsAre("dog-memory"));
}

TEST_F(AIChatEmbeddingsServiceTest,
       LearnedMemoryDeletedWhileEmbeddedIsNotKept) {
  embedder_.Pause();
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  // The sync of the write is embedding the memory.
  ASSERT_TRUE(base::test::RunUntil([&] { return embedder_.HasPendingJobs(); }));

  // The sync of the delete ends before the embedding of the write.
  DeleteLearnedMemory("cat-memory");
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service_->IsLearnedMemoryIndexCurrent(); }));
  embedder_.Resume();
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest,
       RemovesLearnedMemoriesOfDeletedConversations) {
  ConversationHandler* cat_conversation = AddConversation(
      "Tell me about my cat", "Cats sleep for most of the day.");
  const std::string cat_uuid = cat_conversation->get_conversation_uuid();
  const std::string cat_entry =
      *cat_conversation->GetConversationHistory()[0]->uuid;
  ConversationHandler* dog_conversation =
      AddConversation("Tell me about my dog", "Dogs bark at the mail carrier.");
  const std::string dog_entry =
      *dog_conversation->GetConversationHistory()[0]->uuid;
  WriteLearnedMemory("cat-memory", "Has a cat named Tom",
                     {{cat_uuid, cat_entry, 0}});
  WriteLearnedMemory(
      "dog-memory", "Walks the dog daily",
      {{dog_conversation->get_conversation_uuid(), dog_entry, 0}});
  WaitForIndexed("Dogs bark at the mail carrier.");
  WaitForLearnedMemoriesSynced();
  ASSERT_THAT(FindLearnedMemories("cat"), ElementsAre("cat-memory"));

  // The conversation database deletes the memory that came from the
  // conversation.
  ai_chat_service_->DeleteConversation(cat_uuid);
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
  EXPECT_THAT(FindLearnedMemories("dog"), ElementsAre("dog-memory"));

  ai_chat_service_->DeleteConversations();
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("dog").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, LearnedMemoriesFollowTheMemorySetting) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  WaitForLearnedMemoriesSynced();

  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);
  WaitForLearnedMemoriesSynced();
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);
  WaitForLearnedMemoriesSynced();

  // The embedding was deleted, and is computed again.
  EXPECT_EQ(CountEmbedded("Has a cat named Tom"), 2u);
  EXPECT_THAT(FindLearnedMemories("cat"), ElementsAre("cat-memory"));
}

TEST_F(AIChatEmbeddingsServiceTest, RemovesLearnedMemoriesWhenStorageIsOff) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  WaitForLearnedMemoriesSynced();

  prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest,
       SyncsLearnedMemoriesChangedWhileNotRunning) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  WaitForLearnedMemoriesSynced();
  DestroyService();
  DeleteLearnedMemory("cat-memory");
  WriteLearnedMemory("dog-memory", "Walks the dog daily");

  CreateService();
  WaitForLearnedMemoriesSynced();
  EXPECT_TRUE(FindLearnedMemories("cat").empty());
  EXPECT_THAT(FindLearnedMemories("dog"), ElementsAre("dog-memory"));
}

TEST_F(AIChatEmbeddingsServiceTest, LearnedMemoryIndexIsCurrentAfterTheSync) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  EXPECT_FALSE(service_->IsLearnedMemoryIndexCurrent());

  base::test::TestFuture<void> current;
  service_->WhenLearnedMemoryIndexCurrent(current.GetCallback());
  ASSERT_TRUE(current.Wait());
  EXPECT_TRUE(service_->IsLearnedMemoryIndexCurrent());
  EXPECT_THAT(FindLearnedMemories("cat"), ElementsAre("cat-memory"));

  // When the index is current, the callback runs at once (later).
  base::test::TestFuture<void> again;
  service_->WhenLearnedMemoryIndexCurrent(again.GetCallback());
  EXPECT_FALSE(again.IsReady());
  EXPECT_TRUE(again.Wait());
}

TEST_F(AIChatEmbeddingsServiceTest, EmbedsLearnedMemoriesAgainForNewModel) {
  WriteLearnedMemory("cat-memory", "Has a cat named Tom");
  WaitForLearnedMemoriesSynced();

  embedder_metadata_provider_.SetMetadata(passage_embeddings::EmbedderMetadata(
      /*model_version=*/2, /*output_size=*/4, /*search_score_threshold=*/0.5));
  ASSERT_TRUE(base::test::RunUntil([&] {
    return CountEmbedded("Has a cat named Tom") > 1 &&
           service_->IsIndexingIdleForTesting();
  }));
  Flush();
  EXPECT_THAT(FindLearnedMemories("cat"), ElementsAre("cat-memory"));
}

TEST_F(AIChatEmbeddingsServiceTest, DeletesIndexWhenSemanticSearchIsOff) {
  AddConversation("Tell me about my cat", "Cats sleep for most of the day.");
  WaitForIndexed("Cats sleep for most of the day.");
  ASSERT_TRUE(base::PathExists(db_file_path()));

  prefs_.SetBoolean(local_ai::prefs::kBraveHistoryEmbeddingsEnabled, false);
  Flush();
  EXPECT_FALSE(base::PathExists(db_file_path()));
  EXPECT_TRUE(SearchConversations("cat").empty());
}

TEST_F(AIChatEmbeddingsServiceTest, DeletesIndexOfStoppedService) {
  AddConversation("Tell me about my cat", "Cats sleep for most of the day.");
  WaitForIndexed("Cats sleep for most of the day.");
  DestroyService();
  ASSERT_TRUE(base::PathExists(db_file_path()));

  AIChatEmbeddingsService::DeleteIndex(temp_dir_.GetPath());
  base::ThreadPoolInstance::Get()->FlushForTesting();
  EXPECT_FALSE(base::PathExists(db_file_path()));
}

}  // namespace ai_chat
