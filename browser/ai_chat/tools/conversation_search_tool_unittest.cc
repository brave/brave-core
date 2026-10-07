// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/conversation_search_tool.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

std::string RunTool(ConversationSearchTool& tool, const std::string& json) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool.UseTool(json, future.GetCallback());
  return ExtractText(future.Get<std::vector<mojom::ContentBlockPtr>>());
}

ConversationPassageMatch MakePassage(std::string text,
                                     base::Time created_time) {
  ConversationPassageMatch passage;
  passage.entry_uuid = "entry";
  passage.created_time = created_time;
  passage.text = std::move(text);
  return passage;
}

}  // namespace

class ConversationSearchToolTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  // Without a service, as once semantic search is turned off.
  ConversationSearchTool tool_{base::WeakPtr<AIChatEmbeddingsService>(),
                               "current"};
};

TEST_F(ConversationSearchToolTest, AsksForPermissionOnce) {
  mojom::ToolUseEvent event(tool_.Name().data(), "1", "{}", std::nullopt,
                            std::nullopt, nullptr, false);
  auto result = tool_.RequiresUserInteractionBeforeHandling(event);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(result));
  EXPECT_TRUE(std::get<mojom::PermissionChallengePtr>(result));

  tool_.UserPermissionGranted("1");
  result = tool_.RequiresUserInteractionBeforeHandling(event);
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE(std::get<bool>(result));
}

TEST_F(ConversationSearchToolTest, RejectsInvalidInput) {
  EXPECT_EQ(RunTool(tool_, "not json"), "Error: failed to parse input JSON");
  EXPECT_EQ(RunTool(tool_, "{}"), "Error: missing or empty 'query' field");
  EXPECT_EQ(RunTool(tool_, R"({"query": ""})"),
            "Error: missing or empty 'query' field");
}

TEST_F(ConversationSearchToolTest, ReportsUnavailableSearch) {
  EXPECT_EQ(RunTool(tool_, R"({"query": "rust lifetimes"})"),
            "Error: searching past conversations is unavailable");
}

TEST_F(ConversationSearchToolTest, RequiresQuery) {
  EXPECT_EQ(tool_.RequiredProperties(), std::vector<std::string>{"query"});
  std::optional<base::DictValue> properties = tool_.InputProperties();
  ASSERT_TRUE(properties);
  EXPECT_TRUE(properties->FindDict("query"));
  EXPECT_TRUE(properties->FindDict("count"));
}

TEST(ConversationSearchToolJsonTest, SerializesResults) {
  const base::Time updated_time = base::Time::FromTimeT(1700000000);
  ConversationSearchResult result;
  result.conversation_uuid = "uuid";
  result.title = "Rust lifetimes";
  result.updated_time = updated_time;
  result.passages.push_back(
      MakePassage("Lifetimes say how long a borrow is valid.", updated_time));
  result.passages.push_back(MakePassage("Untimed", base::Time()));
  std::vector<ConversationSearchResult> results;
  results.push_back(std::move(result));

  std::optional<base::DictValue> root = base::JSONReader::ReadDict(
      internal::BuildConversationSearchResultJson("borrowing", results),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(root);
  EXPECT_EQ(*root->FindString("query"), "borrowing");
  const base::ListValue* conversations = root->FindList("results");
  ASSERT_TRUE(conversations);
  ASSERT_EQ(conversations->size(), 1u);
  const base::DictValue& conversation = (*conversations)[0].GetDict();
  EXPECT_EQ(*conversation.FindString("title"), "Rust lifetimes");
  EXPECT_EQ(*conversation.FindString("last_updated"),
            "2023-11-14T22:13:20.000Z");
  const base::ListValue* passages = conversation.FindList("passages");
  ASSERT_TRUE(passages);
  ASSERT_EQ(passages->size(), 2u);
  EXPECT_EQ(*(*passages)[0].GetDict().FindString("text"),
            "Lifetimes say how long a borrow is valid.");
  EXPECT_EQ(*(*passages)[0].GetDict().FindString("time"),
            "2023-11-14T22:13:20.000Z");
  // A passage without a time, such as one of a title, gives none.
  EXPECT_FALSE((*passages)[1].GetDict().FindString("time"));
  // The conversation's uuid is of no use to the model.
  EXPECT_FALSE(conversation.contains("conversation_uuid"));
}

TEST(ConversationSearchToolJsonTest, SerializesCards) {
  ConversationSearchResult matched;
  matched.conversation_uuid = "uuid-1";
  matched.title = "Rust lifetimes";
  matched.passages.push_back(MakePassage("Best passage", base::Time()));
  matched.passages.push_back(MakePassage("Another passage", base::Time()));
  ConversationSearchResult title_only;
  title_only.conversation_uuid = "uuid-2";
  title_only.title = "Cats";
  std::vector<ConversationSearchResult> results;
  results.push_back(std::move(matched));
  results.push_back(std::move(title_only));

  std::optional<base::ListValue> cards = base::JSONReader::ReadList(
      internal::BuildConversationSearchArtifactJson(results),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(cards);
  ASSERT_EQ(cards->size(), 2u);
  const base::DictValue& first = (*cards)[0].GetDict();
  EXPECT_EQ(*first.FindString("conversation_uuid"), "uuid-1");
  EXPECT_EQ(*first.FindString("title"), "Rust lifetimes");
  // The card shows the passage that matched best.
  EXPECT_EQ(*first.FindString("entry_uuid"), "entry");
  EXPECT_EQ(*first.FindString("snippet"), "Best passage");
  // A conversation found by its title has no passage to open it at.
  const base::DictValue& second = (*cards)[1].GetDict();
  EXPECT_EQ(*second.FindString("conversation_uuid"), "uuid-2");
  EXPECT_EQ(*second.FindString("title"), "Cats");
  EXPECT_FALSE(second.contains("entry_uuid"));
  EXPECT_FALSE(second.contains("snippet"));
}

TEST(ConversationSearchToolJsonTest, SerializesNoResults) {
  std::optional<base::DictValue> root = base::JSONReader::ReadDict(
      internal::BuildConversationSearchResultJson("anything", {}),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(root);
  const base::ListValue* conversations = root->FindList("results");
  ASSERT_TRUE(conversations);
  EXPECT_TRUE(conversations->empty());
}

}  // namespace ai_chat
