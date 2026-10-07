// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/memory_semantic_search_tool.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

std::string RunTool(MemorySemanticSearchTool& tool, const std::string& json) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool.UseTool(json, future.GetCallback());
  const auto& blocks = future.Get<std::vector<mojom::ContentBlockPtr>>();
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

mojom::ModelPtr CreateModel(std::optional<std::string> system_prompt) {
  auto model = mojom::Model::New();
  model->key = "model";
  model->supports_tools = true;
  if (system_prompt) {
    auto options = mojom::CustomModelOptions::New();
    options->model_system_prompt = std::move(system_prompt);
    model->options =
        mojom::ModelOptions::NewCustomModelOptions(std::move(options));
  } else {
    model->options =
        mojom::ModelOptions::NewLeoModelOptions(mojom::LeoModelOptions::New());
  }
  return model;
}

}  // namespace

class MemorySemanticSearchToolTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  // Without a service, as once semantic search is turned off.
  MemorySemanticSearchTool tool_{base::WeakPtr<AIChatEmbeddingsService>()};
};

TEST_F(MemorySemanticSearchToolTest, RejectsInvalidInput) {
  EXPECT_EQ(RunTool(tool_, "not json"), "Error: failed to parse input JSON");
  EXPECT_EQ(RunTool(tool_, "{}"), "Error: missing or empty 'query' field");
}

TEST_F(MemorySemanticSearchToolTest, ReportsUnavailableSearch) {
  EXPECT_EQ(RunTool(tool_, R"({"query": "food"})"),
            "Error: searching memories is unavailable");
}

TEST_F(MemorySemanticSearchToolTest, AsksForNoPermission) {
  mojom::ToolUseEvent event(tool_.Name().data(), "1", "{}", std::nullopt,
                            std::nullopt, nullptr, false);
  auto result = tool_.RequiresUserInteractionBeforeHandling(event);
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE(std::get<bool>(result));
}

// Matches when memories go along with a request.
TEST_F(MemorySemanticSearchToolTest, OfferedWhereMemoriesAreSent) {
  EXPECT_TRUE(tool_.SupportsConversation(/*is_temporary=*/false,
                                         /*has_untrusted_content=*/true, {}));
  EXPECT_FALSE(tool_.SupportsConversation(/*is_temporary=*/true,
                                          /*has_untrusted_content=*/false, {}));

  EXPECT_TRUE(tool_.IsSupportedByModel(*CreateModel(std::nullopt), {}));
  EXPECT_TRUE(tool_.IsSupportedByModel(*CreateModel(""), {}));
  EXPECT_FALSE(tool_.IsSupportedByModel(*CreateModel("Be brief."), {}));
}

TEST(MemorySemanticSearchToolJsonTest, SerializesMemories) {
  std::optional<base::DictValue> root = base::JSONReader::ReadDict(
      internal::BuildMemorySearchResultJson(
          "food", {"Is vegetarian", "Dislikes cilantro"}),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(root);
  EXPECT_EQ(*root->FindString("query"), "food");
  const base::ListValue* memories = root->FindList("memories");
  ASSERT_TRUE(memories);
  ASSERT_EQ(memories->size(), 2u);
  EXPECT_EQ((*memories)[0].GetString(), "Is vegetarian");
  EXPECT_EQ((*memories)[1].GetString(), "Dislikes cilantro");
  // Without learned memories, the output is as it was before they existed.
  EXPECT_FALSE(root->contains("learned_memories"));
}

TEST(MemorySemanticSearchToolJsonTest, SerializesLearnedMemoriesApart) {
  std::optional<base::DictValue> root = base::JSONReader::ReadDict(
      internal::BuildMemorySearchResultJson("food", {"Is vegetarian"},
                                            {"Cooks pasta on Sundays"}),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(root);
  const base::ListValue* memories = root->FindList("memories");
  ASSERT_TRUE(memories);
  ASSERT_EQ(memories->size(), 1u);
  EXPECT_EQ((*memories)[0].GetString(), "Is vegetarian");
  const base::ListValue* learned = root->FindList("learned_memories");
  ASSERT_TRUE(learned);
  ASSERT_EQ(learned->size(), 1u);
  EXPECT_EQ((*learned)[0].GetString(), "Cooks pasta on Sundays");
}

}  // namespace ai_chat
