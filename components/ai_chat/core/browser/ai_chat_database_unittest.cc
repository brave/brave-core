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
    db_ = std::make_unique<AIChatDatabase>();
    ASSERT_TRUE(db_->Init(db_file_path()));
  }

  void TearDown() override {
    db_.reset();
    ASSERT_TRUE(temp_directory_.Delete());
  }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().AppendASCII("ai_chat");
  }

 protected:
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<AIChatDatabase> db_;
};

TEST_F(AIChatDatabaseTest, AddConversations) {
  int64_t first_id = db_->AddConversation(mojom::Conversation::New(
      INT64_C(1), base::Time::Now(), "Initial conversation", GURL()));
  EXPECT_EQ(first_id, INT64_C(1));
  std::vector<mojom::ConversationPtr> conversations =
      db_->GetAllConversations();
  EXPECT_EQ(conversations.size(), UINT64_C(1));
  int64_t second_id = db_->AddConversation(mojom::Conversation::New(
      INT64_C(2), base::Time::Now(), "Another conversation", GURL()));
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

  static const char kFirstResponse[] = "This is a generated response";
  static const char kSecondResponse[] = "This is a re-generated response";

  static const base::Time kFirstTextCreatedAt(base::Time::Now());
  static const base::Time kSecondTextCreatedAt(base::Time::Now() +
                                               base::Minutes(5));

  static std::vector<mojom::ConversationEntryTextPtr> kTexts;
  kTexts.emplace_back(mojom::ConversationEntryText::New(1, kFirstTextCreatedAt,
                                                        kFirstResponse));

  // Add conversation
  int64_t conversation_id = db_->AddConversation(mojom::Conversation::New(
      kConversationId, kFirstTextCreatedAt, kConversationTitle, kURL));
  EXPECT_EQ(conversation_id, kConversationId);

  // Add conversation entry
  int64_t entry_id = db_->AddConversationEntry(
      conversation_id, mojom::ConversationEntry::New(
                           INT64_C(-1), kFirstTextCreatedAt,
                           mojom::CharacterType::ASSISTANT, std::move(kTexts)));
  EXPECT_EQ(entry_id, kConversationEntryId);

  // Get conversations
  std::vector<mojom::ConversationPtr> conversations =
      db_->GetAllConversations();
  EXPECT_FALSE(conversations.empty());
  EXPECT_EQ(conversations[0]->id, INT64_C(1));
  EXPECT_EQ(conversations[0]->title, kConversationTitle);
  EXPECT_EQ(conversations[0]->page_url->spec(), kURL.spec());
  // Conversation's date must match first text's date
  EXPECT_EQ(GetInternalValue(conversations[0]->date),
            GetInternalValue(kFirstTextCreatedAt));

  // Get conversation entries
  std::vector<mojom::ConversationEntryPtr> entries =
      db_->GetConversationEntries(conversation_id);
  EXPECT_EQ(UINT64_C(1), entries.size());
  EXPECT_EQ(entries[0]->character_type, mojom::CharacterType::ASSISTANT);
  EXPECT_EQ(GetInternalValue(entries[0]->date),
            GetInternalValue(kFirstTextCreatedAt));
  EXPECT_EQ(UINT64_C(1), entries[0]->texts.size());
  EXPECT_EQ(entries[0]->texts[0]->text, kFirstResponse);

  // Add another text to first entry
  db_->AddConversationEntryText(
      entries[0]->id, mojom::ConversationEntryText::New(
                          UINT64_C(1), kSecondTextCreatedAt, kSecondResponse));
  entries = db_->GetConversationEntries(conversation_id);
  EXPECT_EQ(UINT64_C(2), entries[0]->texts.size());
  EXPECT_EQ(entries[0]->texts[1]->text, kSecondResponse);
}

}  // namespace ai_chat
