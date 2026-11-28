// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

// Uses the default tool-specific model checks.
class TestTool : public Tool {
 public:
  TestTool() = default;
  ~TestTool() override = default;

  std::string_view Name() const override { return "test_tool"; }
  std::string_view Description() const override { return "A test tool"; }

  void UseTool(const std::string& input_json,
               UseToolCallback callback) override {}
};

mojom::ModelPtr CreateModel(
    bool supports_tools,
    std::vector<mojom::ConversationCapability> supported_capabilities) {
  auto model = mojom::Model::New();
  model->key = "test-model";
  model->display_name = "Test Model";
  model->supports_tools = supports_tools;
  model->supported_capabilities = std::move(supported_capabilities);
  return model;
}

}  // namespace

TEST(ToolTest, IsSupportedByModel_RequiresToolSupport) {
  TestTool tool;
  auto model = CreateModel(/*supports_tools=*/false,
                           {mojom::ConversationCapability::DEEP_RESEARCH});
  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH}));
}

TEST(ToolTest, IsSupportedByModel_RequiresModelToDeclareCapability) {
  TestTool tool;
  auto model = CreateModel(/*supports_tools=*/true,
                           {mojom::ConversationCapability::DEEP_RESEARCH});

  EXPECT_TRUE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH}));

  // Model doesn't declare CONTENT_AGENT, so it gets no tools. The conversation
  // also requires a capability the model does declare, so every capability has
  // to match, not just one.
  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH,
               mojom::ConversationCapability::CONTENT_AGENT}));
}

TEST(ToolTest, IsSupportedByModel_ServerHintsDoNotGate) {
  TestTool tool;
  // No model declares MATH_ML, so requiring it would filter out every tool.
  auto model = CreateModel(/*supports_tools=*/true, {});

  EXPECT_TRUE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::MATH_ML}));
}

TEST(ToolTest, IsSupportedByModel_PermissiveToolRequiresToolSupport) {
  MockTool permissive_tool("test_tool");
  const Tool& tool = permissive_tool;
  auto model = CreateModel(/*supports_tools=*/false,
                           {mojom::ConversationCapability::DEEP_RESEARCH});

  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH}));
  EXPECT_FALSE(tool.IsSupportedByModel(*model, {}));
}

TEST(ToolTest, IsSupportedByModel_PermissiveToolRequiresEveryCapability) {
  MockTool permissive_tool("test_tool");
  const Tool& tool = permissive_tool;
  auto model = CreateModel(/*supports_tools=*/true,
                           {mojom::ConversationCapability::DEEP_RESEARCH});

  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::CONTENT_AGENT}));
  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH,
               mojom::ConversationCapability::CONTENT_AGENT}));
}

TEST(ToolTest, IsSupportedByModel_ToolSpecificChecksCanReject) {
  MockTool restrictive_tool("test_tool");
  const Tool& tool = restrictive_tool;
  auto model = CreateModel(/*supports_tools=*/true,
                           {mojom::ConversationCapability::DEEP_RESEARCH});

  EXPECT_TRUE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH}));
  restrictive_tool.set_is_supported_by_model(false);
  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::DEEP_RESEARCH}));
}

TEST(ToolTest, IsSupportedByModel_ServerHintsCannotBypassToolSupport) {
  MockTool permissive_tool("test_tool");
  const Tool& tool = permissive_tool;
  auto model = CreateModel(/*supports_tools=*/false, {});

  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::MATH_ML,
               mojom::ConversationCapability::WORKSPACES}));
}

TEST(ToolTest, IsSupportedByModel_ToolSpecificChecksAllowServerHints) {
  MockTool permissive_tool("test_tool");
  const Tool& tool = permissive_tool;
  auto model = CreateModel(/*supports_tools=*/true, {});

  EXPECT_TRUE(tool.IsSupportedByModel(*model, {}));
  EXPECT_TRUE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::MATH_ML,
               mojom::ConversationCapability::WORKSPACES}));
  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::MATH_ML,
               mojom::ConversationCapability::CONTENT_AGENT}));
}

}  // namespace ai_chat
