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
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "sql/init_status.h"
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

    // Create database when os_crypt is ready
    base::RunLoop run_loop;
    encryptor_ready_subscription_ =
        os_crypt_->GetInstance(base::BindLambdaForTesting(
            [&](os_crypt_async::Encryptor encryptor, bool success) {
              ASSERT_TRUE(success);
              db_ = std::make_unique<AIChatDatabase>(db_file_path(),
                                                     std::move(encryptor));
              run_loop.Quit();
            }));
    run_loop.Run();

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
  base::CallbackListSubscription encryptor_ready_subscription_;
  std::unique_ptr<AIChatDatabase> db_;
  base::FilePath path_;
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
    const std::string expected_contents = "Page contents";
    auto details =
        mojom::SiteInfoDetail::New(page_url, "page title", page_url.host(),
                                   mojom::ContentType::PageContent);
    std::vector<mojom::SiteInfoDetailPtr> details_vector;
    details_vector.push_back(details->Clone());
    mojom::SiteInfoPtr associated_content =
        has_content
            ? mojom::SiteInfo::New(content_uuid, std::move(details_vector), 62,
                                   true, true)
            : mojom::SiteInfo::New(std::nullopt,
                                   std::vector<mojom::SiteInfoDetailPtr>(), 0,
                                   false, false);
    const mojom::ConversationPtr metadata =
        mojom::Conversation::New(uuid, "title", now - base::Hours(2), true,
                                 std::nullopt, std::move(associated_content));

    // Persist the first entry (and get the response ready)
    auto history = CreateSampleChatHistory(1u);

    EXPECT_TRUE(db_->AddConversation(
        metadata->Clone(),
        has_content ? std::make_optional(expected_contents) : std::nullopt,
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
      EXPECT_EQ(result->associated_content[0]->content, expected_contents);
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
          mojom::ConversationTurnVisibility::VISIBLE, "edited query 1",
          std::nullopt, std::nullopt, base::Time::Now() + base::Minutes(121),
          std::nullopt, false));
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
          mojom::ConversationTurnVisibility::VISIBLE, "edited query 2",
          std::nullopt, std::nullopt, base::Time::Now() + base::Minutes(122),
          std::nullopt, false));
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

TEST_P(AIChatDatabaseTest, UpdateConversationTitle) {
  const std::vector<std::string> initial_titles = {"first title", ""};
  for (const auto& initial_title : initial_titles) {
    const std::string uuid =
        base::StrCat({"for_conversation_title_", initial_title});
    const std::string updated_title = "updated title";
    mojom::ConversationPtr metadata = mojom::Conversation::New(
        uuid, initial_title, base::Time::Now(), true, std::nullopt,
        mojom::SiteInfo::New(std::nullopt,
                             std::vector<mojom::SiteInfoDetailPtr>(), 0, false,
                             false));

    // Persist the first entry (and get the response ready)
    const auto history = CreateSampleChatHistory(1u);

    EXPECT_TRUE(db_->AddConversation(metadata->Clone(), std::nullopt,
                                     history[0]->Clone()));

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

TEST_P(AIChatDatabaseTest, AddOrUpdateAssociatedContent) {
  const std::string uuid = "for_associated_content";
  const std::string content_uuid = "content_uuid";
  const GURL page_url = GURL("https://example.com/page");
  auto details = mojom::SiteInfoDetail::New(
      page_url, "page title", page_url.host(), mojom::ContentType::PageContent);
  std::vector<mojom::SiteInfoDetailPtr> details_vector;
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt,
      mojom::SiteInfo::New(content_uuid, std::move(details_vector), 62, true,
                           true));

  auto history = CreateSampleChatHistory(1u);

  std::string expected_contents = "First contents";
  EXPECT_TRUE(db_->AddConversation(metadata->Clone(),
                                   std::make_optional(expected_contents),
                                   history[0]->Clone()));

  // Verify data is persisted
  mojom::ConversationArchivePtr result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 1u);
  EXPECT_EQ(result->associated_content[0]->content_uuid, content_uuid);
  EXPECT_EQ(result->associated_content[0]->content, expected_contents);
  auto conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata);

  // Change data and call AddOrUpdateAssociatedContent
  expected_contents = "Second contents";
  metadata->associated_content->content_used_percentage = 50;
  metadata->associated_content->is_content_refined = false;
  EXPECT_TRUE(db_->AddOrUpdateAssociatedContent(
      uuid, metadata->associated_content->Clone(),
      std::make_optional(expected_contents)));
  // Verify data is changed
  result = db_->GetConversationData(uuid);
  EXPECT_EQ(result->associated_content.size(), 1u);
  EXPECT_EQ(result->associated_content[0]->content_uuid,
            metadata->associated_content->uuid.value());
  EXPECT_EQ(result->associated_content[0]->content, expected_contents);
  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 1u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata);
}

TEST_P(AIChatDatabaseTest, DeleteAllData) {
  const std::string uuid = "first";
  const GURL page_url = GURL("https://example.com/page");
  mojom::ConversationPtr metadata = mojom::Conversation::New(
      uuid, "title", base::Time::Now() - base::Hours(2), true, std::nullopt,
      mojom::SiteInfo::New(std::nullopt,
                           std::vector<mojom::SiteInfoDetailPtr>(), 0, false,
                           false));

  auto history = CreateSampleChatHistory(1u);

  EXPECT_TRUE(db_->AddConversation(metadata->Clone(), std::nullopt,
                                   history[0]->Clone()));

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
  std::string expected_contents = "First contents";

  // The times in the Conversation are irrelevant, only the times of the entries
  // are persisted.
  auto details = mojom::SiteInfoDetail::New(
      page_url, "page title", page_url.host(), mojom::ContentType::PageContent);
  std::vector<mojom::SiteInfoDetailPtr> details_vector;
  details_vector.push_back(details->Clone());
  mojom::ConversationPtr metadata_first = mojom::Conversation::New(
      "first", "title", base::Time::Now() - base::Hours(2), true, std::nullopt,
      mojom::SiteInfo::New("first-content", std::move(details_vector), 62, true,
                           true));

  details_vector.push_back(details->Clone());
  mojom::ConversationPtr metadata_second = mojom::Conversation::New(
      "second", "title", base::Time::Now() - base::Hours(1), true, "model-2",
      mojom::SiteInfo::New("second-content", std::move(details_vector), 62,
                           true, true));

  auto history_first = CreateSampleChatHistory(1u, -2);
  auto history_second = CreateSampleChatHistory(1u, -1);

  EXPECT_TRUE(db_->AddConversation(metadata_first->Clone(),
                                   std::make_optional(expected_contents),
                                   history_first[0]->Clone()));

  EXPECT_TRUE(db_->AddConversation(metadata_second->Clone(),
                                   std::make_optional(expected_contents),
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
  EXPECT_EQ(archive_result->associated_content[0]->content, expected_contents);
  archive_result = db_->GetConversationData("second");
  EXPECT_EQ(archive_result->associated_content.size(), 1u);
  EXPECT_EQ(archive_result->associated_content[0]->content_uuid,
            "second-content");
  EXPECT_EQ(archive_result->associated_content[0]->content, expected_contents);

  // Delete associated content to only consider the second conversation
  EXPECT_TRUE(db_->DeleteAssociatedWebContent(
      base::Time::Now() + base::Minutes(-61), std::nullopt));

  // Verify only url, title and content was deleted and only from the second
  // conversation
  conversations = db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), 2u);
  ExpectConversationEquals(FROM_HERE, conversations[0], metadata_first);
  metadata_second->associated_content->details.clear();
  ExpectConversationEquals(FROM_HERE, conversations[1], metadata_second);

  archive_result = db_->GetConversationData("second");
  EXPECT_TRUE(archive_result->associated_content.empty());
  archive_result = db_->GetConversationData("first");
  EXPECT_EQ(archive_result->associated_content.size(), 1u);
  EXPECT_EQ(archive_result->associated_content[0]->content_uuid,
            "first-content");
  EXPECT_EQ(archive_result->associated_content[0]->content, expected_contents);
}

}  // namespace ai_chat
