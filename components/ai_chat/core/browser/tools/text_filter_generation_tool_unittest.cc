// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/text_filter_generation_tool.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

// Mock AssociatedContentManager for testing
class MockAssociatedContentManager : public AssociatedContentManager {
 public:
  explicit MockAssociatedContentManager()
      : AssociatedContentManager(nullptr) {}

  void SetMockContent(std::vector<mojom::AssociatedContentPtr> content,
                      std::vector<PageContents> cached_contents) {
    mock_content_ = std::move(content);
    mock_cached_contents_ = std::move(cached_contents);
  }

  std::vector<mojom::AssociatedContentPtr> GetAssociatedContent()
      const override {
    std::vector<mojom::AssociatedContentPtr> result;
    for (const auto& content : mock_content_) {
      result.push_back(content.Clone());
    }
    return result;
  }

  const std::vector<std::reference_wrapper<const PageContents>>
  GetCachedContents() const override {
    std::vector<std::reference_wrapper<const PageContents>> result;
    for (const auto& content : mock_cached_contents_) {
      result.push_back(std::cref(content));
    }
    return result;
  }

  bool HasAssociatedContent() const override {
    return !mock_content_.empty();
  }

 private:
  std::vector<mojom::AssociatedContentPtr> mock_content_;
  std::vector<PageContents> mock_cached_contents_;
};

class TextFilterGenerationToolTest : public testing::Test {
 public:
  TextFilterGenerationToolTest() = default;
  ~TextFilterGenerationToolTest() override = default;

  void SetUp() override {
    mock_content_manager_ = std::make_unique<MockAssociatedContentManager>();
    filter_tool_ = std::make_unique<TextFilterGenerationTool>(
        mock_content_manager_.get());
  }

  void TearDown() override {
    filter_tool_.reset();
    mock_content_manager_.reset();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<MockAssociatedContentManager> mock_content_manager_;
  std::unique_ptr<TextFilterGenerationTool> filter_tool_;
};

TEST_F(TextFilterGenerationToolTest, UseTool_ValidInput) {
  // Setup mock content with DOM structure
  auto content = mojom::AssociatedContent::New();
  content->url = GURL("https://example.com/page");
  std::vector<mojom::AssociatedContentPtr> content_vec;
  content_vec.push_back(std::move(content));

  PageContents page_contents;
  page_contents.content = "Page content here";
  page_contents.dom_structure = R"([
    {"tag": "div", "class": "cookie-banner", "text": "Accept cookies"}
  ])";

  std::vector<PageContents> cached_contents;
  cached_contents.push_back(std::move(page_contents));

  mock_content_manager_->SetMockContent(std::move(content_vec),
                                        std::move(cached_contents));

  const std::string input_json =
      R"({"user_request": "Hide the cookie banner"})";

  base::test::TestFuture<Tool::ToolResult> future;
  filter_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  // Parse the result JSON
  std::string result_text = result[0]->get_text_content_block()->text;
  std::optional<base::Value> parsed = base::JSONReader::Read(result_text);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->is_dict());

  const base::Value::Dict& result_dict = parsed->GetDict();
  const std::string* prompt = result_dict.FindString("prompt");
  const std::string* domain = result_dict.FindString("domain");
  const std::string* user_request = result_dict.FindString("user_request");

  ASSERT_TRUE(prompt);
  ASSERT_TRUE(domain);
  ASSERT_TRUE(user_request);

  EXPECT_EQ(*domain, "example.com");
  EXPECT_EQ(*user_request, "Hide the cookie banner");
  EXPECT_TRUE(prompt->find("cookie-banner") != std::string::npos);
  EXPECT_TRUE(prompt->find("PAGE STRUCTURE") != std::string::npos);
}

TEST_F(TextFilterGenerationToolTest, UseTool_InvalidJson) {
  const std::string input_json = R"(invalid json)";

  base::test::TestFuture<Tool::ToolResult> future;
  filter_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Invalid input - Failed to parse JSON");
}

TEST_F(TextFilterGenerationToolTest, UseTool_MissingUserRequest) {
  const std::string input_json = R"({"other_field": "value"})";

  base::test::TestFuture<Tool::ToolResult> future;
  filter_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Invalid input - Missing user_request field");
}

TEST_F(TextFilterGenerationToolTest, UseTool_EmptyUserRequest) {
  const std::string input_json = R"({"user_request": ""})";

  base::test::TestFuture<Tool::ToolResult> future;
  filter_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Invalid input - Missing user_request field");
}

TEST_F(TextFilterGenerationToolTest, UseTool_NoAssociatedContent) {
  // Don't set any mock content - empty by default
  const std::string input_json =
      R"({"user_request": "Hide the cookie banner"})";

  base::test::TestFuture<Tool::ToolResult> future;
  filter_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  std::string error_text = result[0]->get_text_content_block()->text;
  EXPECT_TRUE(error_text.find("No page content available") != std::string::npos);
}

TEST_F(TextFilterGenerationToolTest, UseTool_NoDOMStructure) {
  // Setup content without DOM structure
  auto content = mojom::AssociatedContent::New();
  content->url = GURL("https://example.com/page");
  std::vector<mojom::AssociatedContentPtr> content_vec;
  content_vec.push_back(std::move(content));

  PageContents page_contents;
  page_contents.content = "Page content here";
  // No dom_structure set

  std::vector<PageContents> cached_contents;
  cached_contents.push_back(std::move(page_contents));

  mock_content_manager_->SetMockContent(std::move(content_vec),
                                        std::move(cached_contents));

  const std::string input_json =
      R"({"user_request": "Hide the cookie banner"})";

  base::test::TestFuture<Tool::ToolResult> future;
  filter_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  std::string error_text = result[0]->get_text_content_block()->text;
  EXPECT_TRUE(error_text.find("DOM structure not available") != std::string::npos);
}

TEST_F(TextFilterGenerationToolTest, SupportsConversation_WithContent) {
  EXPECT_TRUE(filter_tool_->SupportsConversation(
      false, true, mojom::ConversationCapability::CHAT));
}

TEST_F(TextFilterGenerationToolTest, SupportsConversation_WithoutContent) {
  EXPECT_FALSE(filter_tool_->SupportsConversation(
      false, false, mojom::ConversationCapability::CHAT));
}

TEST_F(TextFilterGenerationToolTest, SupportsConversation_Temporary) {
  // Tool should work even in temporary conversations if content is available
  EXPECT_TRUE(filter_tool_->SupportsConversation(
      true, true, mojom::ConversationCapability::CHAT));
}

TEST_F(TextFilterGenerationToolTest, ToolMetadata) {
  EXPECT_EQ(filter_tool_->Name(), "text_filter_generation");
  EXPECT_FALSE(filter_tool_->Description().empty());

  auto input_props = filter_tool_->InputProperties();
  ASSERT_TRUE(input_props.has_value());

  auto required_props = filter_tool_->RequiredProperties();
  ASSERT_TRUE(required_props.has_value());
  EXPECT_EQ(required_props->size(), 1u);
  EXPECT_EQ((*required_props)[0], "user_request");
}

}  // namespace ai_chat
