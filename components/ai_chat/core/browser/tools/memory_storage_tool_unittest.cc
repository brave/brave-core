// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/memory_storage_tool.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-data-view.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ai_chat {

class MemoryStorageToolTest : public testing::Test {
 public:
  MemoryStorageToolTest() = default;
  ~MemoryStorageToolTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());
    memory_tool_ = std::make_unique<MemoryStorageTool>(&pref_service_);
  }

  void TearDown() override { memory_tool_.reset(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<MemoryStorageTool> memory_tool_;
};

TEST_F(MemoryStorageToolTest, UseTool_ValidInput) {
  const std::string input_json = R"({"memory": "User prefers TypeScript"})";

  base::test::TestFuture<Tool::ToolResult> future;
  memory_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  // Should return empty string on success
  EXPECT_EQ(result[0]->get_text_content_block()->text, "");

  // Verify memory was stored in prefs
  const base::Value::List& memories =
      pref_service_.GetList(prefs::kBraveAIChatUserMemories);
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].GetString(), "User prefers TypeScript");
}

TEST_F(MemoryStorageToolTest, UseTool_InvalidJson) {
  const std::string input_json = R"({"memory": invalid json})";

  base::test::TestFuture<Tool::ToolResult> future;
  memory_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Invalid JSON input, input must be a JSON object");

  // Verify no memory was stored
  const base::Value::List& memories =
      pref_service_.GetList(prefs::kBraveAIChatUserMemories);
  EXPECT_EQ(memories.size(), 0u);
}

TEST_F(MemoryStorageToolTest, UseTool_MissingMemoryField) {
  const std::string input_json = R"({"other_field": "value"})";

  base::test::TestFuture<Tool::ToolResult> future;
  memory_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Missing or empty 'memory' field");

  // Verify no memory was stored
  const base::Value::List& memories =
      pref_service_.GetList(prefs::kBraveAIChatUserMemories);
  EXPECT_EQ(memories.size(), 0u);
}

TEST_F(MemoryStorageToolTest, UseTool_EmptyMemoryField) {
  const std::string input_json = R"({"memory": ""})";

  base::test::TestFuture<Tool::ToolResult> future;
  memory_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Missing or empty 'memory' field");

  // Verify no memory was stored
  const base::Value::List& memories =
      pref_service_.GetList(prefs::kBraveAIChatUserMemories);
  EXPECT_EQ(memories.size(), 0u);
}

TEST_F(MemoryStorageToolTest, UseTool_TooLongMemory) {
  // Create a string longer than kMaxMemoryRecordLength characters
  std::string long_memory(mojom::kMaxMemoryRecordLength + 1, 'a');
  const std::string input_json =
      absl::StrFormat(R"({"memory": "%s"})", long_memory);

  base::test::TestFuture<Tool::ToolResult> future;
  memory_tool_->UseTool(input_json, future.GetCallback());

  Tool::ToolResult result = future.Take();
  ASSERT_EQ(result.size(), 1u);
  ASSERT_TRUE(result[0]->is_text_content_block());

  EXPECT_EQ(result[0]->get_text_content_block()->text,
            "Error: Memory content exceeds 512 character limit");

  // Verify no memory was stored
  const base::Value::List& memories =
      pref_service_.GetList(prefs::kBraveAIChatUserMemories);
  EXPECT_EQ(memories.size(), 0u);
}

TEST_F(MemoryStorageToolTest, SupportsConversation_NonTemporary) {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  EXPECT_FALSE(memory_tool_->SupportsConversation(
      false, false, mojom::ConversationCapability::CHAT));
#else
  EXPECT_TRUE(memory_tool_->SupportsConversation(
      false, false, mojom::ConversationCapability::CHAT));
#endif
}

TEST_F(MemoryStorageToolTest, SupportsConversation_Temporary) {
  EXPECT_FALSE(memory_tool_->SupportsConversation(
      true, false, mojom::ConversationCapability::CHAT));
}

TEST_F(MemoryStorageToolTest, SupportsConversation_UntrustedContent) {
  EXPECT_FALSE(memory_tool_->SupportsConversation(
      false, true, mojom::ConversationCapability::CHAT));
}

}  // namespace ai_chat
