// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/ai_chat_database.h"

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_tree.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {
class AIChatDatabaseTest : public testing::Test,
                           public testing::WithParamInterface<bool> {
 public:
  AIChatDatabaseTest() = default;

  void SetUp() override {
    CHECK(temp_directory_.CreateUniqueTempDir());
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    base::test::TestFuture<os_crypt_async::Encryptor> future;
    os_crypt_->GetInstance(future.GetCallback());
    db_ = std::make_unique<AIChatDatabase>(db_file_path(), future.Take());

    if (GetParam()) {
      db_->DeleteAllData();
    }
  }

  void TearDown() override {
    // Verify that the db was init successfully and not using default return
    // values.
    EXPECT_TRUE(IsInitOk());

    db_.reset();
    CHECK(temp_directory_.Delete());
  }

  bool IsInitOk() {
    return (db_->db_init_status_.has_value() &&
            db_->db_init_status_.value() == sql::InitStatus::INIT_OK);
  }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().AppendASCII("ai_chat");
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  std::unique_ptr<AIChatDatabase> db_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatDatabaseTest,
    // Run all tests with the initial schema and the schema created
    // after calling DeleteAllData, to verify the schemas are the same
    // and no tables are missing or different.
    ::testing::Bool(),
    [](const testing::TestParamInfo<AIChatDatabaseTest::ParamType>& info) {
      return base::StringPrintf("DropTablesFirst_%s",
                                info.param ? "Yes" : "No");
    });

// Functions tested:
// - AddConversation
// - GetAllConversations
// - GetConversationData
// - AddConversationEntry
// - DeleteConversationEntry
// - DeleteConversation
TEST_P(AIChatDatabaseTest, AddAndGetConversationAndEntries) {
  auto now = base::Time::Now();

  // Do this for both associated content and non-associated content
  for (bool has_content : {true, false}) {
    SCOPED_TRACE(testing::Message() << (has_content ? "With" : "Without")
                                    << " associated content");
    const std::string uuid = has_content ? "first" : "second";
    const std::string content_uuid = "content";
    // Create the conversation metadata which gets persisted
    // when the first entry is asked to be persisted.
    // Put an incorrect time value to show that the time from the
    // mojom::Conversation is not persisted and instead is taken from the most
    // recent entry.
    const GURL page_url = GURL("https://example.com/page");
    const std::vector<std::string> expected_contents = {"Page contents"};
    std::vector<mojom::AssociatedContentPtr> associated_content;
    if (has_content) {
      associated_content.push_back(mojom::AssociatedContent::New(
          content_uuid, mojom::ContentType::PageContent, "page title", 1,
          page_url, 62));
    }
    const mojom::ConversationPtr metadata = mojom::Conversation::New(
        uuid, "title", now - base::Hours(2), true, std::nullopt, 0, 0, false,
        std::move(associated_content));

    // Persist the first entry (and get the response ready)
    auto history = CreateSampleChatHistory(1u);
    // Edit the prompt to show that the prompt is persisted
    history[0]->prompt = "first entry prompt";

    EXPECT_TRUE(db_->AddConversation(
        metadata->Clone(),
        has_content ? expected_contents : std::vector<std::string>(),
        history[0]->Clone()));

    // Test getting the conversation metadata
    std::vector<mojom::ConversationPtr> conversations =
        db_->GetAllConversations();
    EXPECT_EQ(conversations.size(), has_content ? 1u : 2u);
    auto& conversation = has_content ? conversations[0] : conversations[1];
    ExpectConversationEquals(FROM_HERE, conversation, metadata);
    EXPECT_EQ(conversation->updated_time, history.front()->created_time);

    // Persist the response entry
    EXPECT_TRUE(db_->AddConversationEntry(uuid, history[1]->Clone()));

    // Test getting the conversation entries
    mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);
    ExpectConversationHistoryEquals(FROM_HERE, result->entries, history);
    EXPECT_EQ(result->associated_content.size(), has_content ? 1u : 0u);
    if (has_content) {
      EXPECT_EQ(result->associated_content[0]->content_uuid, content_uuid);
      EXPECT_EQ(result->associated_content[0]->content, expected_contents[0]);
    }

    // Add another pair of entries
    auto next_history = CreateSampleChatHistory(1u, 1);
    // Change the model this time
    std::string new_model_key = "model-2";
    EXPECT_TRUE(db_->AddConversationEntry(uuid, next_history[0]->Clone(),
                                          new_model_key));
    EXPECT_TRUE(db_->AddConversationEntry(uuid, next_history[1]->Clone()));

    // Verify all entries are returned
    mojom::ConversationArchivePtr result_2 = db_->GetConversationData(uuid);
    for (auto& entry : next_history) {
      history.push_back(std::move(entry));
    }
    ExpectConversationHistoryEquals(FROM_HERE, result_2->entries, history);

    // Verify metadata now has new model key
    conversations = db_->GetAllConversations();
    EXPECT_EQ(conversations.size(), has_content ? 1u : 2u);
    ExpectConversationEquals(
        FROM_HERE, has_content ? conversations[0] : conversations[1], metadata);

    // Edits (delete, re-add and check edit re-construction)

    // Delete the last response
    EXPECT_TRUE(
        db_->DeleteConversationEntry(result_2->entries.back()->uuid.value()));

    // Verify the last entry is gone
    history.pop_back();
    mojom::ConversationArchivePtr result_3 = db_->GetConversationData(uuid);
    ExpectConversationHistoryEquals(FROM_HERE, result_3->entries, history);

    // Add an edit to the last query
    {
      auto& last_query = result_3->entries.back();
      last_query->edits = std::vector<mojom::ConversationTurnPtr>{};
      last_query->edits->emplace_back(mojom::ConversationTurn::New(
          base::Uuid::GenerateRandomV4().AsLowercaseString(),
          mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
          "edited query 1", std::nullopt, std::nullopt, std::nullopt,
          base::Time::Now() + base::Minutes(121), std::nullopt, std::nullopt,
          false, std::nullopt));
      EXPECT_TRUE(db_->DeleteConversationEntry(last_query->uuid.value()));
      EXPECT_TRUE(db_->AddConversationEntry(uuid, last_query->Clone()));
    }
    // Verify the edit is persisted
    mojom::ConversationArchivePtr result_4 = db_->GetConversationData(uuid);
    ExpectConversationHistoryEquals(FROM_HERE, result_4->entries,
                                    result_3->entries);

    // Add another edit to test multiple edits for the same turn
    {
      auto& last_query = result_4->entries.back();
      last_query->edits->emplace_back(mojom::ConversationTurn::New(
          base::Uuid::GenerateRandomV4().AsLowercaseString(),
          mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
          "edited query 2", std::nullopt, std::nullopt, std::nullopt,
          base::Time::Now() + base::Minutes(122), std::nullopt, std::nullopt,
          false, std::nullopt));
      EXPECT_TRUE(db_->DeleteConversationEntry(last_query->uuid.value()));
      EXPECT_TRUE(db_->AddConversationEntry(uuid, last_query->Clone()));
    }
    // Verify multiple edits are persisted
    mojom::ConversationArchivePtr result_5 = db_->GetConversationData(uuid);
    ExpectConversationHistoryEquals(FROM_HERE, result_5->entries,
                                    result_4->entries);
  }

  // Test deleting conversation (after loop so that we can test conversation
  // entry selection with multiple conversations in the database).
  EXPECT_TRUE(db_->DeleteConversation("second"));
  // Verify no data for deleted conversation
  mojom::ConversationArchivePtr conversation_data =
      db_->GetConversationData("second");
  EXPECT_EQ(conversation_data->entries.size(), 0u);
  EXPECT_EQ(conversation_data->associated_content.size(), 0u);
  // Verify deleted conversation metadata not returned
  std::vector<mojom::ConversationPtr> conversations =
      db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  EXPECT_EQ(conversations[0]->uuid, "first");
  // Verify there's still data for other conversations
  mojom::ConversationArchivePtr conversation_data_2 =
      db_->GetConversationData("first");
  EXPECT_GT(conversation_data_2->entries.size(), 0u);
  EXPECT_EQ(conversation_data_2->associated_content.size(), 1u);
  // Delete last conversation
  EXPECT_TRUE(db_->DeleteConversation("first"));
  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 0u);
}

TEST_P(AIChatDatabaseTest, WebSourcesEvent) {
  const std::string uuid = "first";
  const GURL page_url = GURL("https://example.com/page");
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt, 0,
      0, false, std::vector<mojom::AssociatedContentPtr>());

  // Test 2 entries to verify they are recorded against different entries
  auto history = CreateSampleChatHistory(2u);
  {
    std::vector<mojom::WebSourcePtr> sources_first;
    sources_first.emplace_back(
        mojom::WebSource::New("title1", GURL("https://example.com/source1"),
                              GURL("https://www.example.com/source1favicon")));
    sources_first.emplace_back(
        mojom::WebSource::New("title2", GURL("https://example.com/source2"),
                              GURL("https://www.example.com/source2favicon")));

    std::vector<mojom::WebSourcePtr> sources_second;
    sources_second.emplace_back(
        mojom::WebSource::New("2title1", GURL("https://2example.com/source1"),
                              GURL("https://www.2example.com/source1favicon")));
    sources_second.emplace_back(
        mojom::WebSource::New("2title2", GURL("https://2example.com/source2"),
                              GURL("https://www.2example.com/source2favicon")));

    history[1]->events->emplace_back(
        mojom::ConversationEntryEvent::NewSourcesEvent(
            mojom::WebSourcesEvent::New(std::move(sources_first))));
    history[3]->events->emplace_back(
        mojom::ConversationEntryEvent::NewSourcesEvent(
            mojom::WebSourcesEvent::New(std::move(sources_second))));
  }

  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[1]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[2]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[3]->Clone()));
  mojom::ConversationArchivePtr conversation_data =
      db_->GetConversationData(uuid);
  ExpectConversationHistoryEquals(FROM_HERE, conversation_data->entries,
                                  history);
}

TEST_P(AIChatDatabaseTest, WebSourcesEvent_Invalid) {
  const std::string uuid = "first";
  const GURL page_url = GURL("https://example.com/page");
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt, 0,
      0, false, std::vector<mojom::AssociatedContentPtr>());

  // Test 2 entries to verify they are recorded against different entries. Make
  // some entries invalid to verify they are not persisted.
  // Test an additional entry with a sources even but no items.
  auto history = CreateSampleChatHistory(3u);
  {
    std::vector<mojom::WebSourcePtr> sources_first;
    sources_first.emplace_back(mojom::WebSource::New(
        "title1", GURL(""), GURL("https://www.example.com/source1favicon")));
    sources_first.emplace_back(
        mojom::WebSource::New("title2", GURL("https://example.com/source2"),
                              GURL("https://www.example.com/source2favicon")));

    std::vector<mojom::WebSourcePtr> sources_second;
    sources_second.emplace_back(
        mojom::WebSource::New("2title1", GURL("https://2example.com/source1"),
                              GURL("https://www.2example.com/source1favicon")));
    sources_second.emplace_back(mojom::WebSource::New(
        "2title2", GURL("https://2example.com/source2"), GURL("")));

    history[1]->events->emplace_back(
        mojom::ConversationEntryEvent::NewSourcesEvent(
            mojom::WebSourcesEvent::New(std::move(sources_first))));
    history[3]->events->emplace_back(
        mojom::ConversationEntryEvent::NewSourcesEvent(
            mojom::WebSourcesEvent::New(std::move(sources_second))));
    history[5]->events->emplace_back(
        mojom::ConversationEntryEvent::NewSourcesEvent(
            mojom::WebSourcesEvent::New()));
  }

  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[1]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[2]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[3]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[4]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(uuid, history[5]->Clone()));

  // Update expected data to remove the invalid sources
  history[1]->events->back()->get_sources_event()->sources.erase(
      history[1]->events->back()->get_sources_event()->sources.begin());
  history[3]->events->back()->get_sources_event()->sources.pop_back();
  history[5]->events->pop_back();

  mojom::ConversationArchivePtr conversation_data =
      db_->GetConversationData(uuid);
  ExpectConversationHistoryEquals(FROM_HERE, conversation_data->entries,
                                  history);
}

TEST_P(AIChatDatabaseTest, UploadFile) {
  constexpr char kUUID[] = "upload_file_uuid";
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      kUUID, "title", base::Time::Now() - base::Hours(2), true, std::nullopt, 0,
      0, false, std::vector<mojom::AssociatedContentPtr>());
  auto history = CreateSampleChatHistory(2u, 0, 3u);

  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(kUUID, history[1]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(kUUID, history[2]->Clone()));
  EXPECT_TRUE(db_->AddConversationEntry(kUUID, history[3]->Clone()));
  mojom::ConversationArchivePtr conversation_data =
      db_->GetConversationData(kUUID);
  ExpectConversationHistoryEquals(FROM_HERE, conversation_data->entries,
                                  history);

  ASSERT_EQ(conversation_data->entries.size(), 4u);
  // Delete last pair of history
  EXPECT_TRUE(db_->DeleteConversationEntry(
      conversation_data->entries[3]->uuid.value()));
  EXPECT_TRUE(db_->DeleteConversationEntry(
      conversation_data->entries[2]->uuid.value()));

  history.erase(history.end() - 2, history.end());
  mojom::ConversationArchivePtr conversation_data2 =
      db_->GetConversationData(kUUID);
  ExpectConversationHistoryEquals(FROM_HERE, conversation_data2->entries,
                                  history);

  // Delete the whole conversation contains uploaded image
  EXPECT_TRUE(db_->DeleteConversation(kUUID));
  EXPECT_EQ(db_->GetAllConversations().size(), 0u);
}

TEST_P(AIChatDatabaseTest, UpdateConversationTitle) {
  const std::vector<std::string> initial_titles = {"first title", ""};
  for (const auto& initial_title : initial_titles) {
    const std::string uuid =
        base::StrCat({"for_conversation_title_", initial_title});
    const std::string updated_title = "updated title";
    mojom::ConversationPtr metadata = mojom::Conversation::New(
        uuid, initial_title, base::Time::Now(), true, std::nullopt, 0, 0, false,
        std::vector<mojom::AssociatedContentPtr>());

    // Persist the first entry (and get the response ready)
    const auto history = CreateSampleChatHistory(1u);

    EXPECT_TRUE(
        db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));

    // Verify initial title
    std::vector<mojom::ConversationPtr> conversations =
        db_->GetAllConversations();
    // get this conversation
    auto* conversation = GetConversation(FROM_HERE, conversations, uuid);
    EXPECT_EQ(conversation->title, initial_title);

    // Update title
    EXPECT_TRUE(db_->UpdateConversationTitle(uuid, updated_title));
    // Verify
    conversations = db_->GetAllConversations();
    conversation = GetConversation(FROM_HERE, conversations, uuid);
    EXPECT_EQ(conversation->title, updated_title);
  }
}

TEST_P(AIChatDatabaseTest, UpdateConversationTokenInfo) {
  uint64_t initial_total_tokens = 4000;
  uint64_t initial_trimmed_tokens = 800;
  const std::string uuid = "for_token_info";
  uint64_t updated_total_tokens = 5000;
  uint64_t updated_trimmed_tokens = 1200;
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now(), true, std::nullopt,
      initial_total_tokens, initial_trimmed_tokens, false,
      std::vector<mojom::AssociatedContentPtr>());

  // Persist the first entry (and get the response ready)
  const auto history = CreateSampleChatHistory(1u);
  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));

  // Verify initial token info
  std::vector<mojom::ConversationPtr> conversations =
      db_->GetAllConversations();
  // get this conversation
  auto* conversation = GetConversation(FROM_HERE, conversations, uuid);
  EXPECT_EQ(conversation->total_tokens, initial_total_tokens);
  EXPECT_EQ(conversation->trimmed_tokens, initial_trimmed_tokens);

  // Update token info
  EXPECT_TRUE(db_->UpdateConversationTokenInfo(uuid, updated_total_tokens,
                                               updated_trimmed_tokens));

  // Verify updated token info
  conversations = db_->GetAllConversations();
  conversation = GetConversation(FROM_HERE, conversations, uuid);
  EXPECT_EQ(conversation->total_tokens, updated_total_tokens);
  EXPECT_EQ(conversation->trimmed_tokens, updated_trimmed_tokens);
}

TEST_P(AIChatDatabaseTest, AddOrUpdateAssociatedContent) {
  const std::string uuid = "for_associated_content";
  const std::string content_uuid = "content_uuid";
  const GURL page_url = GURL("https://example.com/page");

  // Note: This is reused for all conversations, as it is moved into the
  // conversation ptr.
  std::vector<mojom::AssociatedContentPtr> associated_content;

  associated_content.push_back(mojom::AssociatedContent::New(
      content_uuid, mojom::ContentType::PageContent, "page title", 1, page_url,
      62));

  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt, 0,
      0, false, std::move(associated_content));

  auto history = CreateSampleChatHistory(1u);

  std::vector<std::string> expected_contents = {"First contents"};
  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), expected_contents,
                                   history[0]->Clone()));

  // Verify data is persisted
  mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 1u);
  EXPECT_EQ(result->associated_content[0]->content_uuid, content_uuid);
  EXPECT_EQ(result->associated_content[0]->content, expected_contents[0]);
  auto conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata);

  // Change data and call AddOrUpdateAssociatedContent
  expected_contents[0] = "Second contents";
  metadata->associated_content[0]->content_used_percentage = 50;

  associated_content.push_back(metadata->associated_content[0]->Clone());
  EXPECT_TRUE(db_->AddOrUpdateAssociatedContent(
      uuid, std::move(associated_content), expected_contents));
  // Verify data is changed
  result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 1u);
  EXPECT_EQ(result->associated_content[0]->content_uuid,
            metadata->associated_content[0]->uuid);
  EXPECT_EQ(result->associated_content[0]->content, expected_contents[0]);
  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata);
}

TEST_P(AIChatDatabaseTest, AddOrUpdateAssociatedContent_MultiContent) {
  const std::string uuid = "for_associated_content";
  auto content_1 = mojom::AssociatedContent::New(
      "content_1", mojom::ContentType::PageContent, "one", 1,
      GURL("https://one.com"), 61);
  auto content_2 = mojom::AssociatedContent::New(
      "content_2", mojom::ContentType::PageContent, "two", 2,
      GURL("https://two.com"), 62);

  // Note: This is reused for all conversations, as it is moved into the
  // conversation ptr.
  std::vector<mojom::AssociatedContentPtr> associated_content;
  associated_content.push_back(content_1->Clone());
  associated_content.push_back(content_2->Clone());

  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt, 0,
      0, false, std::move(associated_content));

  auto history = CreateSampleChatHistory(1u);

  std::vector<std::string> expected_contents = {"First contents",
                                                "Second contents"};
  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), expected_contents,
                                   history[0]->Clone()));

  // Verify data is persisted
  mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 2u);
  EXPECT_EQ(result->associated_content[0]->content_uuid, content_1->uuid);
  EXPECT_EQ(result->associated_content[0]->content, expected_contents[0]);
  EXPECT_EQ(result->associated_content[1]->content_uuid, content_2->uuid);
  EXPECT_EQ(result->associated_content[1]->content, expected_contents[1]);

  auto conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata);

  // Change data and call AddOrUpdateAssociatedContent
  expected_contents[0] = "New first contents";
  metadata->associated_content[1]->title = "Updated title!";

  associated_content.push_back(metadata->associated_content[0]->Clone());
  associated_content.push_back(metadata->associated_content[1]->Clone());

  EXPECT_TRUE(db_->AddOrUpdateAssociatedContent(
      uuid, std::move(associated_content), expected_contents));
  // Verify data is changed
  result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 2u);
  EXPECT_EQ(result->associated_content[0]->content_uuid,
            metadata->associated_content[0]->uuid);
  EXPECT_EQ(result->associated_content[0]->content, expected_contents[0]);
  EXPECT_EQ(result->associated_content[1]->content_uuid,
            metadata->associated_content[1]->uuid);
  EXPECT_EQ(result->associated_content[1]->content, expected_contents[1]);
  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata);

  // Delete the associated content
  EXPECT_TRUE(db_->DeleteAssociatedWebContent(
      base::Time::Now() - base::Hours(3), std::nullopt));

  // Verify the associated content is deleted
  result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 0u);

  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  EXPECT_EQ(conversations[0]->associated_content.size(), 2u);
  EXPECT_EQ(conversations[0]->associated_content[0]->title, "");
  EXPECT_EQ(conversations[0]->associated_content[0]->url, GURL());
  EXPECT_EQ(conversations[0]->associated_content[1]->title, "");
  EXPECT_EQ(conversations[0]->associated_content[1]->url, GURL());
}

TEST_P(AIChatDatabaseTest, DeleteAllData) {
  const std::string uuid = "first";
  const GURL page_url = GURL("https://example.com/page");
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt, 0,
      0, false, std::vector<mojom::AssociatedContentPtr>());

  auto history = CreateSampleChatHistory(1u);

  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));

  // Verify data is persisted
  {
    mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);

    ExpectConversationEntryEquals(FROM_HERE, result->entries[0], history[0]);
    auto conversations = db_->GetAllConversations();
    EXPECT_EQ(conversations.size(), 1u);
    ExpectConversationEquals(FROM_HERE, conversations[0], metadata);
  }

  // Delete all data
  db_->DeleteAllData();

  // Verify no data
  {
    auto conversations = db_->GetAllConversations();
    EXPECT_EQ(conversations.size(), 0u);
    mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);
    EXPECT_EQ(result->entries.size(), 0u);
  }
}

TEST_P(AIChatDatabaseTest, DeleteAssociatedWebContent) {
  GURL page_url = GURL("https://example.com/page");
  std::vector<std::string> expected_contents = {"First contents"};

  // Note: This is reused for all conversations, as it is moved into the
  // conversation ptr. std::move(associated_content) will reset
  // |associated_content| to the empty vector.
  std::vector<mojom::AssociatedContentPtr> associated_content;

  // The times in the Conversation are irrelevant, only the times of the entries
  // are persisted.
  associated_content.push_back(mojom::AssociatedContent::New(
      "first-content", mojom::ContentType::PageContent, "page title", 1,
      page_url, 62));
  mojom::ConversationPtr metadata_first = mojom::Conversation::New(
      "first", "title", base::Time::Now() - base::Hours(2), true, std::nullopt,
      0, 0, false, std::move(associated_content));

  associated_content.push_back(mojom::AssociatedContent::New(
      "second-content", mojom::ContentType::PageContent, "page title", 2,
      page_url, 62));
  mojom::ConversationPtr metadata_second = mojom::Conversation::New(
      "second", "title", base::Time::Now() - base::Hours(1), true, "model-2", 0,
      0, false, std::move(associated_content));

  auto history_first = CreateSampleChatHistory(1u, -2);
  auto history_second = CreateSampleChatHistory(1u, -1);

  EXPECT_TRUE(db_->AddConversation(metadata_first->Clone(), expected_contents,
                                   history_first[0]->Clone()));

  EXPECT_TRUE(db_->AddConversation(metadata_second->Clone(), expected_contents,
                                   history_second[0]->Clone()));

  // Verify data is persisted
  auto conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 2u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata_first);
  ExpectConversationEquals(FROM_HERE, conversations[1], metadata_second);

  mojom::ConversationArchivePtr archive_result =
      db_->GetConversationData("first");
  EXPECT_EQ(archive_result->associated_content.size(), 1u);
  EXPECT_EQ(archive_result->associated_content[0]->content_uuid,
            "first-content");
  EXPECT_EQ(archive_result->associated_content[0]->content,
            expected_contents[0]);
  archive_result = db_->GetConversationData("second");
  EXPECT_EQ(archive_result->associated_content.size(), 1u);
  EXPECT_EQ(archive_result->associated_content[0]->content_uuid,
            "second-content");
  EXPECT_EQ(archive_result->associated_content[0]->content,
            expected_contents[0]);

  // Delete associated content to only consider the second conversation
  EXPECT_TRUE(db_->DeleteAssociatedWebContent(
      base::Time::Now() + base::Minutes(-61), std::nullopt));

  // Verify only url, title and content was deleted and only from the second
  // conversation
  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 2u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata_first);
  metadata_second->associated_content[0]->url = GURL();
  metadata_second->associated_content[0]->title = "";
  ExpectConversationEquals(FROM_HERE, conversations[1], metadata_second);

  archive_result = db_->GetConversationData("second");
  EXPECT_TRUE(archive_result->associated_content.empty());
  archive_result = db_->GetConversationData("first");
  EXPECT_EQ(archive_result->associated_content.size(), 1u);
  EXPECT_EQ(archive_result->associated_content[0]->content_uuid,
            "first-content");
  EXPECT_EQ(archive_result->associated_content[0]->content,
            expected_contents[0]);
}

// Test the migration for each version upgrade
class AIChatDatabaseMigrationTest : public testing::Test,
                                    public testing::WithParamInterface<int> {
 public:
  AIChatDatabaseMigrationTest() = default;

  void SetUp() override {
    OSCryptMocker::SetUp();
    CHECK(temp_directory_.CreateUniqueTempDir());
    database_dump_location_ = database_dump_location_.AppendASCII("brave")
                                  .AppendASCII("test")
                                  .AppendASCII("data")
                                  .AppendASCII("ai_chat");
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    // Create database when os_crypt is ready
    base::test::TestFuture<os_crypt_async::Encryptor> future;
    os_crypt_->GetInstance(future.GetCallback());
    CreateDatabase(
        base::StringPrintf("aichat_database_dump_version_%d.sql", version()));
    db_ = std::make_unique<AIChatDatabase>(db_file_path(), future.Take());
  }

  void TearDown() override {
    // Verify that the db was init successfully and not using default return
    // values.
    EXPECT_TRUE(IsInitOk());
    db_.reset();
    // Verify current version of database is latest
    sql::Database db(sql::Database::Tag("AIChatDatabase"));
    sql::MetaTable meta_table;
    ASSERT_TRUE(db.Open(db_file_path()));
    ASSERT_TRUE(meta_table.Init(&db, kCurrentDatabaseVersion,
                                kCompatibleDatabaseVersionNumber));
    EXPECT_EQ(kCompatibleDatabaseVersionNumber,
              meta_table.GetCompatibleVersionNumber());
    EXPECT_EQ(kCurrentDatabaseVersion, meta_table.GetVersionNumber());
    db.Close();
    OSCryptMocker::TearDown();
    task_environment_.RunUntilIdle();
    ASSERT_TRUE(temp_directory_.Delete());
  }

  // Creates the database from |sql_file|.
  void CreateDatabase(std::string_view sql_file) {
    base::FilePath database_dump =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
    database_dump =
        database_dump.Append(database_dump_location_).AppendASCII(sql_file);
    ASSERT_TRUE(sql::test::CreateDatabaseFromSQL(db_file_path(), database_dump))
        << "Could not create database from sql dump file at: "
        << database_dump.value();
  }

  bool IsInitOk() {
    return db_->db_init_status_.has_value() &&
           db_->db_init_status_.value() == sql::InitStatus::INIT_OK;
  }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().AppendASCII("test_ai_chat.db");
  }

  // Returns the database version for the test.
  int version() const { return GetParam(); }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::FilePath database_dump_location_;
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  std::unique_ptr<AIChatDatabase> db_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    AIChatDatabaseMigrationTest,
    testing::Range(kLowestSupportedDatabaseVersion, kCurrentDatabaseVersion),
    [](const testing::TestParamInfo<AIChatDatabaseMigrationTest::ParamType>&
           info) {
      return base::StringPrintf("From_v%d_to_v%d", info.param,
                                kCurrentDatabaseVersion);
    });

// Tests the migration of the database from version() to kCurrentVersionNumber
TEST_P(AIChatDatabaseMigrationTest, MigrationToVCurrent) {
  if (version() == kCurrentDatabaseVersion) {
    return;
  }

  // V2 Specific Migration checks
  {
    // Verify we have existing entries
    auto conversations = db_->GetAllConversations();
    EXPECT_GT(conversations.size(), 0u);
    EXPECT_GT(db_->GetConversationData(conversations[0]->uuid)->entries.size(),
              0u);
    // ConversationEntry table changed, check it persists correctly
    auto now = base::Time::Now();
    const std::string uuid = "migrationtest";
    const mojom::ConversationPtr metadata = mojom::Conversation::New(
        uuid, "title", now - base::Hours(2), true, std::nullopt, 0, 0, false,
        std::vector<mojom::AssociatedContentPtr>());

    // Persist the first entry (and get the response ready)
    auto history = CreateSampleChatHistory(1u);
    // Edit the prompt to show that the prompt is persisted
    history[0]->prompt = "first entry prompt";

    EXPECT_TRUE(
        db_->AddConversation(metadata->Clone(), {}, history[0]->Clone()));
    // Persist the response entry
    EXPECT_TRUE(db_->AddConversationEntry(uuid, history[1]->Clone()));

    // Test getting the conversation entries
    mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);
    ExpectConversationHistoryEquals(FROM_HERE, result->entries, history);

    // Add another pair of entries
    auto next_history = CreateSampleChatHistory(1u, 1);
    EXPECT_TRUE(db_->AddConversationEntry(uuid, next_history[0]->Clone()));
    EXPECT_TRUE(db_->AddConversationEntry(uuid, next_history[1]->Clone()));

    // Verify all entries are returned
    mojom::ConversationArchivePtr result_2 = db_->GetConversationData(uuid);
    for (auto& entry : next_history) {
      history.push_back(std::move(entry));
    }
    ExpectConversationHistoryEquals(FROM_HERE, result_2->entries, history);
  }

  // V3 Specific Migration checks
  {
    // Make sure conversations have the default value after migration
    auto existing_conversations = db_->GetAllConversations();
    ASSERT_EQ(existing_conversations.size(), 3u);
    EXPECT_EQ(existing_conversations[0]->total_tokens, 0u);
    EXPECT_EQ(existing_conversations[0]->trimmed_tokens, 0u);
    EXPECT_EQ(existing_conversations[1]->total_tokens, 0u);
    EXPECT_EQ(existing_conversations[1]->trimmed_tokens, 0u);
    // This one was created above and not from
    // test/data/ai_chat/aichat_database_dump_version_[%i].sql
    EXPECT_EQ(existing_conversations[2]->total_tokens, 0u);
    EXPECT_EQ(existing_conversations[2]->trimmed_tokens, 0u);

    // Create site info with the new token field
    const std::string uuid = "migrationtest2";
    const std::string content_uuid = "content_uuid";
    std::vector<mojom::AssociatedContentPtr> associated_content;
    associated_content.push_back(mojom::AssociatedContent::New(
        content_uuid, mojom::ContentType::PageContent, "test title", 1,
        GURL("https://example.com"), 62));

    uint64_t expected_total_tokens = 3770;
    uint64_t expected_trimmed_tokens = 100;
    auto metadata = mojom::Conversation::New(
        uuid, "title", base::Time::Now(), true, std::nullopt,
        expected_total_tokens, expected_trimmed_tokens, false,
        std::move(associated_content));

    // Add a conversation entry
    auto history = CreateSampleChatHistory(1u);
    std::vector<std::string> expected_contents = {"Test content"};
    EXPECT_TRUE(db_->AddConversation(metadata->Clone(), expected_contents,
                                     history[0]->Clone()));

    // Verify the token values were properly stored after migration
    auto conversations = db_->GetAllConversations();
    auto* test_conversation = GetConversation(FROM_HERE, conversations, uuid);
    EXPECT_EQ(test_conversation->total_tokens, expected_total_tokens);
    EXPECT_EQ(test_conversation->trimmed_tokens, expected_trimmed_tokens);
  }

  // V4 Specific Migration checks
  {
    if (version() == 3) {
      auto conversation_with_image =
          db_->GetConversationData("1ae484fe-ab33-4f42-8813-14080e4addc1");
      ASSERT_TRUE(conversation_with_image);
      ASSERT_EQ(conversation_with_image->entries.size(), 2u);
      ASSERT_TRUE(conversation_with_image->entries[0]->uploaded_files);
      ASSERT_TRUE(conversation_with_image->entries[1]->uploaded_files);
      ASSERT_EQ(conversation_with_image->entries[0]->uploaded_files->size(),
                2u);
      ASSERT_EQ(conversation_with_image->entries[1]->uploaded_files->size(),
                1u);
      EXPECT_EQ(
          conversation_with_image->entries[0]->uploaded_files->at(0)->type,
          mojom::UploadedFileType::kImage);
      EXPECT_EQ(
          conversation_with_image->entries[0]->uploaded_files->at(1)->type,
          mojom::UploadedFileType::kImage);
      EXPECT_EQ(
          conversation_with_image->entries[1]->uploaded_files->at(0)->type,
          mojom::UploadedFileType::kImage);
    }

    // Verify the newly added entry with files after migration have `type`
    // persisted.
    auto history = CreateSampleChatHistory(1u, 0, 3u);
    EXPECT_TRUE(
        db_->AddConversationEntry("migrationtest2", history[0]->Clone()));
    auto test_conversation = db_->GetConversationData("migrationtest2");
    ASSERT_EQ(test_conversation->entries.size(), 2u);
    ASSERT_TRUE(test_conversation->entries[1]->uploaded_files);
    ASSERT_TRUE(history[0]->uploaded_files);
    EXPECT_EQ(test_conversation->entries[1]->uploaded_files->size(),
              history[0]->uploaded_files->size());
    for (size_t i = 0;
         i < test_conversation->entries[1]->uploaded_files->size(); ++i) {
      EXPECT_EQ(test_conversation->entries[1]->uploaded_files->at(i)->type,
                history[0]->uploaded_files->at(i)->type);
    }
  }

  // V5 Specific Migration checks
  {
    // Verify existing entries have model_key field added with NULL.
    if (version() == 4) {
      // Get conversation with specific UUID
      auto conversation_data =
          db_->GetConversationData("1ae484fe-ab33-4f42-8813-14080e4addc1");
      ASSERT_TRUE(conversation_data);
      ASSERT_GT(conversation_data->entries.size(), 0u);

      // Check all entries in this conversation have null model_key (default
      // value)
      for (const auto& entry : conversation_data->entries) {
        EXPECT_FALSE(entry->model_key.has_value()) << *entry->uuid;
      }
    }

    // Create a new conversation with two entries.
    const std::string uuid = "model_key_test";
    const std::string conversation_model_key = "automatic";
    mojom::ConversationPtr metadata = mojom::Conversation::New(
        uuid, "title", base::Time::Now(), true, conversation_model_key, 0, 0,
        false, std::vector<mojom::AssociatedContentPtr>());

    auto history = CreateSampleChatHistory(1u);
    EXPECT_TRUE(db_->AddConversation(
        metadata->Clone(), std::vector<std::string>(), history[0]->Clone()));

    EXPECT_TRUE(db_->AddConversationEntry(uuid, history[1]->Clone(),
                                          conversation_model_key));

    // Verify model_keys are stored correctly in both conversation and entry
    // level.
    // 1. Conversation-level model_key persistence
    auto conversations = db_->GetAllConversations();
    auto* conversation = GetConversation(FROM_HERE, conversations, uuid);
    EXPECT_EQ(conversation->model_key, conversation_model_key);

    // 2. Entry-level model_key persistence
    const std::string entry_model_key = "chat-basic";
    auto conversation_data = db_->GetConversationData(uuid);
    ASSERT_EQ(conversation_data->entries.size(), 2u);
    EXPECT_FALSE(conversation_data->entries[0]->model_key);
    EXPECT_EQ(conversation_data->entries[1]->model_key, entry_model_key);
  }

  // V6 Specific Migration checks
  {
    // Nothing really to check here, just making sure we can still load data.
    // This migration only drops a column, and its removed from the model too.
    if (version() == 5) {
      // Get conversation with specific UUID
      auto conversation_data =
          db_->GetConversationData("1ae484fe-ab33-4f42-8813-14080e4addc1");
      ASSERT_TRUE(conversation_data);
    }

    const std::string uuid = "is_content_refined_test";

    mojom::ConversationPtr metadata = mojom::Conversation::New(
        uuid, "title", base::Time::Now(), true, std::nullopt, 0, 0, false,
        std::vector<mojom::AssociatedContentPtr>());
    metadata->associated_content.push_back(mojom::AssociatedContent::New(
        "1234", mojom::ContentType::PageContent, "title", 1,
        GURL("https://example.com"), 100));

    auto history = CreateSampleChatHistory(1u);
    EXPECT_TRUE(db_->AddConversation(metadata->Clone(), {"Hello world!"},
                                     history[0]->Clone()));

    auto data = db_->GetConversationData(uuid);
    ASSERT_TRUE(data);
    EXPECT_EQ(data->associated_content.size(), 1u);
    EXPECT_EQ(data->associated_content[0]->content, "Hello world!");
  }
}

}  // namespace ai_chat
