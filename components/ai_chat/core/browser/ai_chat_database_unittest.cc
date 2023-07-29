/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_database.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
int64_t GetInternalValue(const base::Time& time) {
  return time.ToDeltaSinceWindowsEpoch().InMicroseconds();
}
}  // namespace

namespace ai_chat {

class AIChatDatabaseTest : public testing::Test {
 public:
  AIChatDatabaseTest() = default;

  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    // We take ownership of the path for adhoc inspection of the spitted
    // db. This is useful to test sql queries. If you do this, make sure to
    // comment out the base::DeletePathRecursively line.
    path_ = temp_directory_.Take();
    db_ = std::make_unique<AIChatDatabase>();
    ASSERT_TRUE(db_->Init(db_file_path()));
  }

  void TearDown() override {
    db_.reset();
    ASSERT_TRUE(base::DeletePathRecursively(path_));
  }

  base::FilePath db_file_path() { return path_.AppendASCII("ai_chat"); }

 protected:
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<AIChatDatabase> db_;
  base::FilePath path_;
};

TEST_F(AIChatDatabaseTest, AddConversations) {
  int64_t first_id = db_->AddConversation(mojom::Conversation::New(
      INT64_C(1), base::Time::Now(), "Initial conversation", GURL(),
      "Sample page content"));
  EXPECT_EQ(first_id, INT64_C(1));
  std::vector<mojom::ConversationPtr> conversations =
      db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), UINT64_C(1));
  int64_t second_id = db_->AddConversation(mojom::Conversation::New(
      INT64_C(2), base::Time::Now(), "Another conversation", GURL(),
      "Another sample page content"));
  EXPECT_EQ(second_id, INT64_C(2));
  std::vector<mojom::ConversationPtr> conversations_updated =
      db_->GetAllConversations();
  EXPECT_EQ(conversations_updated.size(), UINT64_C(2));
}

TEST_F(AIChatDatabaseTest, AddConversationEntries) {
  static const int64_t kConversationId = INT64_C(1);
  static const int64_t kConversationEntryId = INT64_C(1);

  static const GURL kURL = GURL("https://brave.com/");
  static const char kConversationTitle[] = "Summarizing Brave";
  static const char kPageContents[] = "Brave is a web browser.";
  static const char kFirstResponse[] = "This is a generated response";
  static const char kSecondResponse[] = "This is a re-generated response";

  static const base::Time kFirstTextCreatedAt(base::Time::Now());
  static const base::Time kSecondTextCreatedAt(base::Time::Now() +
                                               base::Minutes(5));

  // Add conversation
  int64_t conversation_id = db_->AddConversation(
      mojom::Conversation::New(kConversationId, kFirstTextCreatedAt,
                               kConversationTitle, kURL, kPageContents));
  EXPECT_EQ(conversation_id, kConversationId);

  // Add first conversation entry
  {
    std::vector<mojom::ConversationEntryTextPtr> texts;
    texts.emplace_back(mojom::ConversationEntryText::New(1, kFirstTextCreatedAt,
                                                         kFirstResponse));

    std::vector<std::string> search_queries{"brave", "browser", "web"};
    std::vector<mojom::ConversationEntryEventPtr> events;
    events.emplace_back(mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        mojom::SearchQueriesEvent::New(std::move(search_queries))));

    int64_t entry_id = db_->AddConversationEntry(
        conversation_id,
        mojom::ConversationEntry::New(
            INT64_C(-1), kFirstTextCreatedAt, mojom::CharacterType::ASSISTANT,
            mojom::ActionType::UNSPECIFIED, std::nullopt /* selected_text */,
            std::move(texts), std::move(events)));
    EXPECT_EQ(entry_id, kConversationEntryId);
  }

  // Get and verify conversations
  {
    std::vector<mojom::ConversationPtr> conversations =
        db_->GetAllConversations();
    EXPECT_FALSE(conversations.empty());
    EXPECT_EQ(conversations[0]->id, INT64_C(1));
    EXPECT_EQ(conversations[0]->title, kConversationTitle);
    EXPECT_EQ(conversations[0]->page_url->spec(), kURL.spec());
    EXPECT_EQ(conversations[0]->page_contents, kPageContents);
    // A conversation is created with the first entry's date
    EXPECT_EQ(GetInternalValue(conversations[0]->date),
              GetInternalValue(kFirstTextCreatedAt));
  }

  // Get and verify conversation entries
  {
    std::vector<mojom::ConversationEntryPtr> entries =
        db_->GetConversationEntries(conversation_id);
    EXPECT_EQ(UINT64_C(1), entries.size());
    EXPECT_EQ(entries[0]->character_type, mojom::CharacterType::ASSISTANT);
    EXPECT_EQ(GetInternalValue(entries[0]->date),
              GetInternalValue(kFirstTextCreatedAt));
    EXPECT_EQ(UINT64_C(1), entries[0]->texts.size());
    EXPECT_EQ(entries[0]->texts[0]->text, kFirstResponse);
    EXPECT_EQ(entries[0]->action_type, mojom::ActionType::UNSPECIFIED);
    EXPECT_TRUE(entries[0]->selected_text.value().empty());

    // Verify search queries
    EXPECT_TRUE(entries.front()->events.has_value());
    EXPECT_EQ(UINT64_C(3), entries[0]
                               ->events.value()[0]
                               ->get_search_queries_event()
                               ->search_queries.size());
  }

  // Add another text to the first entry
  db_->AddConversationEntryText(
      UINT64_C(1), mojom::ConversationEntryText::New(
                       UINT64_C(1), kSecondTextCreatedAt, kSecondResponse));

  {
    auto entries = db_->GetConversationEntries(conversation_id);
    EXPECT_EQ(UINT64_C(2), entries[0]->texts.size());
    EXPECT_EQ(entries[0]->texts[1]->text, kSecondResponse);
  }

  // Add another entry
  {
    base::Time created_at(base::Time::Now());
    std::string text("Is this a question?");
    std::string selected_text("The brown fox jumps over the lazy dog");

    std::vector<mojom::ConversationEntryTextPtr> texts;
    texts.emplace_back(mojom::ConversationEntryText::New(1, created_at, text));

    int64_t entry_id = db_->AddConversationEntry(
        conversation_id,
        mojom::ConversationEntry::New(
            INT64_C(-1), created_at, mojom::CharacterType::HUMAN,
            mojom::ActionType::CASUALIZE, selected_text, std::move(texts),
            std::nullopt /* search_query */));

    EXPECT_EQ(entry_id, 2);
  }

  // Get and verify conversation entries
  {
    const std::vector<mojom::ConversationEntryPtr>& entries =
        db_->GetConversationEntries(conversation_id);
    EXPECT_EQ(UINT64_C(2), entries.size());
    EXPECT_EQ(entries[1]->selected_text.value(),
              "The brown fox jumps over the lazy dog");
  }
}

}  // namespace ai_chat
