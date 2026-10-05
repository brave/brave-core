// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_database.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatEmbeddingsDatabaseTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    Open(/*model_version=*/1, /*passage_version=*/1);
  }

 protected:
  void Open(int64_t model_version, int passage_version) {
    db_.reset();
    db_ = std::make_unique<AIChatEmbeddingsDatabase>(
        db_file_path(), model_version, passage_version, encryptor_);
  }

  base::FilePath db_file_path() const {
    return temp_dir_.GetPath().Append(kAIChatEmbeddingsDatabaseFileName);
  }

  // All test embeddings are unit vectors in three dimensions; the query points
  // along the first.
  static passage_embeddings::Embedding Query() {
    return passage_embeddings::Embedding({1.0f, 0.0f, 0.0f});
  }

  static ConversationPassage Passage(
      std::string entry_uuid,
      std::string text,
      std::vector<float> embedding,
      std::optional<std::string> thread_uuid = std::nullopt,
      base::Time created_time = base::Time::UnixEpoch()) {
    return ConversationPassage(std::move(entry_uuid), std::move(thread_uuid),
                               created_time, std::move(text),
                               std::move(embedding));
  }

  static std::vector<ConversationPassage> Passages(
      ConversationPassage passage) {
    std::vector<ConversationPassage> passages;
    passages.push_back(std::move(passage));
    return passages;
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_ =
      os_crypt_async::GetTestEncryptorForTesting();
  std::unique_ptr<AIChatEmbeddingsDatabase> db_;
};

TEST_F(AIChatEmbeddingsDatabaseTest, RanksConversationsByBestPassage) {
  const base::Time created_time = base::Time::UnixEpoch() + base::Days(1);
  std::vector<ConversationPassage> passages;
  passages.push_back(Passage("a1", "about the query", {1.0f, 0.0f, 0.0f},
                             "thread", created_time));
  passages.push_back(Passage("a2", "unrelated", {0.0f, 1.0f, 0.0f}));
  ASSERT_TRUE(
      db_->ReplaceConversationPassages("a", std::move(passages), base::Time()));
  ASSERT_TRUE(db_->ReplaceConversationPassages(
      "b", Passages(Passage("b1", "near the query", {0.6f, 0.8f, 0.0f})),
      base::Time()));

  std::vector<ConversationMatch> matches = db_->SearchConversations(
      Query(), /*min_score=*/0.5f, /*count=*/10, /*max_passages=*/3,
      /*excluded_conversation_uuid=*/"");
  ASSERT_EQ(matches.size(), 2u);
  EXPECT_EQ(matches[0].conversation_uuid, "a");
  EXPECT_FLOAT_EQ(matches[0].score, 1.0f);
  // The passage scoring below the minimum is left out.
  ASSERT_EQ(matches[0].passages.size(), 1u);
  EXPECT_EQ(matches[0].passages[0].entry_uuid, "a1");
  EXPECT_EQ(matches[0].passages[0].thread_uuid, "thread");
  EXPECT_EQ(matches[0].passages[0].created_time, created_time);
  EXPECT_EQ(matches[0].passages[0].text, "about the query");
  EXPECT_EQ(matches[1].conversation_uuid, "b");
  EXPECT_FLOAT_EQ(matches[1].score, 0.6f);
  EXPECT_EQ(matches[1].passages[0].text, "near the query");
  EXPECT_FALSE(matches[1].passages[0].thread_uuid.has_value());

  matches = db_->SearchConversations(Query(), 0.5f, 10, 3, "a");
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].conversation_uuid, "b");

  matches = db_->SearchConversations(Query(), 0.5f, /*count=*/1, 3, "");
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].conversation_uuid, "a");

  matches = db_->SearchConversations(Query(), -1.0f, 1, /*max_passages=*/1, "");
  ASSERT_EQ(matches.size(), 1u);
  ASSERT_EQ(matches[0].passages.size(), 1u);
  EXPECT_EQ(matches[0].passages[0].entry_uuid, "a1");

  EXPECT_TRUE(db_->SearchConversations(Query(), 1.1f, 10, 3, "").empty());
}

TEST_F(AIChatEmbeddingsDatabaseTest, ReplacesAndDeletesEntryPassages) {
  const base::Time later = base::Time::UnixEpoch() + base::Days(2);
  const base::Time earlier = base::Time::UnixEpoch() + base::Days(1);
  ASSERT_TRUE(db_->ReplaceEntryPassages(
      "a", "a1", Passages(Passage("a1", "first", {1.0f, 0.0f, 0.0f})), later));
  ASSERT_TRUE(db_->ReplaceEntryPassages(
      "a", "a1", Passages(Passage("a1", "second", {1.0f, 0.0f, 0.0f})),
      earlier));
  // The indexed time only moves forward.
  EXPECT_EQ(db_->GetIndexedConversations(),
            (base::flat_map<std::string, base::Time>{{"a", later}}));

  std::vector<ConversationMatch> matches =
      db_->SearchConversations(Query(), 0.5f, 10, 3, "");
  ASSERT_EQ(matches.size(), 1u);
  ASSERT_EQ(matches[0].passages.size(), 1u);
  EXPECT_EQ(matches[0].passages[0].text, "second");

  // A title passage leaves a new conversation to be indexed in full.
  ASSERT_TRUE(db_->ReplaceEntryPassages(
      "b", "", Passages(Passage("", "title", {1.0f, 0.0f, 0.0f})),
      std::nullopt));
  EXPECT_EQ(db_->GetIndexedConversations().at("b"), base::Time());

  ASSERT_TRUE(db_->DeleteEntryPassages("a", "a1"));
  matches = db_->SearchConversations(Query(), 0.5f, 10, 3, "");
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].conversation_uuid, "b");
}

TEST_F(AIChatEmbeddingsDatabaseTest, ReplacesConversationPassages) {
  ASSERT_TRUE(db_->ReplaceEntryPassages(
      "a", "a1", Passages(Passage("a1", "old", {1.0f, 0.0f, 0.0f})),
      std::nullopt));
  const base::Time indexed_time = base::Time::UnixEpoch() + base::Days(1);
  ASSERT_TRUE(db_->ReplaceConversationPassages(
      "a", Passages(Passage("a2", "new", {1.0f, 0.0f, 0.0f})), indexed_time));

  EXPECT_EQ(db_->GetIndexedConversations(),
            (base::flat_map<std::string, base::Time>{{"a", indexed_time}}));
  std::vector<ConversationMatch> matches =
      db_->SearchConversations(Query(), 0.5f, 10, 3, "");
  ASSERT_EQ(matches.size(), 1u);
  ASSERT_EQ(matches[0].passages.size(), 1u);
  EXPECT_EQ(matches[0].passages[0].entry_uuid, "a2");

  ASSERT_TRUE(db_->InvalidateConversation("a"));
  EXPECT_EQ(db_->GetIndexedConversations().at("a"), base::Time());
}

TEST_F(AIChatEmbeddingsDatabaseTest, DeletesConversations) {
  for (const char* uuid : {"a", "b", "c"}) {
    ASSERT_TRUE(db_->ReplaceConversationPassages(
        uuid, Passages(Passage("1", uuid, {1.0f, 0.0f, 0.0f})),
        base::Time::UnixEpoch()));
  }

  ASSERT_TRUE(db_->DeleteConversation("a"));
  EXPECT_FALSE(db_->GetIndexedConversations().contains("a"));
  EXPECT_EQ(db_->SearchConversations(Query(), 0.5f, 10, 3, "").size(), 2u);

  ASSERT_TRUE(db_->DeleteAllConversations());
  EXPECT_TRUE(db_->GetIndexedConversations().empty());
  EXPECT_TRUE(db_->SearchConversations(Query(), 0.5f, 10, 3, "").empty());
}

TEST_F(AIChatEmbeddingsDatabaseTest, DeletesEmbeddingsOfAnotherVersion) {
  auto add_data = [&] {
    ASSERT_TRUE(db_->ReplaceConversationPassages(
        "a", Passages(Passage("a1", "text", {1.0f, 0.0f, 0.0f})),
        base::Time::UnixEpoch()));
    ASSERT_TRUE(db_->AddMemories({{"memory", {1.0f, 0.0f, 0.0f}}}));
  };
  auto has_data = [&] {
    return !db_->GetIndexedConversations().empty() &&
           !db_->SearchConversations(Query(), 0.5f, 10, 3, "").empty() &&
           !db_->SearchMemories(Query(), 0.5f, 10).empty();
  };
  auto has_no_data = [&] {
    return db_->GetIndexedConversations().empty() &&
           db_->SearchConversations(Query(), -1.0f, 10, 3, "").empty() &&
           db_->SearchMemories(Query(), -1.0f, 10).empty();
  };

  add_data();
  Open(/*model_version=*/1, /*passage_version=*/1);
  EXPECT_TRUE(has_data());

  Open(/*model_version=*/2, /*passage_version=*/1);
  EXPECT_TRUE(has_no_data());

  add_data();
  Open(/*model_version=*/2, /*passage_version=*/2);
  EXPECT_TRUE(has_no_data());
}

TEST_F(AIChatEmbeddingsDatabaseTest, SyncsMemories) {
  EXPECT_EQ(db_->SyncMemories({"likes cats", "likes dogs"}),
            (std::vector<std::string>{"likes cats", "likes dogs"}));
  ASSERT_TRUE(db_->AddMemories({{"likes cats", {1.0f, 0.0f, 0.0f}},
                                {"likes dogs", {0.6f, 0.8f, 0.0f}}}));

  // Stale memories are deleted, and only missing ones are returned, once.
  EXPECT_EQ(db_->SyncMemories({"likes dogs", "likes birds", "likes birds"}),
            std::vector<std::string>{"likes birds"});
  ASSERT_TRUE(db_->AddMemories({{"likes birds", {0.0f, 1.0f, 0.0f}}}));
  EXPECT_TRUE(db_->SyncMemories({"likes dogs", "likes birds"}).empty());

  std::vector<MemoryMatch> matches =
      db_->SearchMemories(Query(), /*min_score=*/-1.0f, /*count=*/10);
  ASSERT_EQ(matches.size(), 2u);
  EXPECT_EQ(matches[0].text, "likes dogs");
  EXPECT_FLOAT_EQ(matches[0].score, 0.6f);
  EXPECT_EQ(matches[1].text, "likes birds");

  matches = db_->SearchMemories(Query(), /*min_score=*/0.5f, /*count=*/10);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].text, "likes dogs");
  EXPECT_EQ(db_->SearchMemories(Query(), -1.0f, /*count=*/1).size(), 1u);

  ASSERT_TRUE(db_->DeleteAllMemories());
  EXPECT_TRUE(db_->SearchMemories(Query(), -1.0f, 10).empty());
}

TEST_F(AIChatEmbeddingsDatabaseTest, EncryptsStoredText) {
  ASSERT_TRUE(db_->ReplaceConversationPassages(
      "a", Passages(Passage("a1", "secret passage", {1.0f, 0.0f, 0.0f})),
      base::Time::UnixEpoch()));
  ASSERT_TRUE(db_->AddMemories({{"secret memory", {1.0f, 0.0f, 0.0f}}}));
  db_.reset();

  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(db_file_path(), &contents));
  EXPECT_FALSE(contents.empty());
  EXPECT_EQ(contents.find("secret passage"), std::string::npos);
  EXPECT_EQ(contents.find("secret memory"), std::string::npos);
}

}  // namespace ai_chat
