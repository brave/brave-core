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
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

// Uses the base IsSupportedByModel, unlike MockTool which overrides it.
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
                           {mojom::ConversationCapability::CHAT});
  EXPECT_FALSE(
      tool.IsSupportedByModel(*model, {mojom::ConversationCapability::CHAT}));
}

TEST(ToolTest, IsSupportedByModel_RequiresModelToDeclareCapability) {
  TestTool tool;
  auto model = CreateModel(/*supports_tools=*/true,
                           {mojom::ConversationCapability::CHAT});

  EXPECT_TRUE(
      tool.IsSupportedByModel(*model, {mojom::ConversationCapability::CHAT}));

  // Model doesn't declare CONTENT_AGENT, so it gets no tools.
  EXPECT_FALSE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::CHAT,
               mojom::ConversationCapability::CONTENT_AGENT}));
}

TEST(ToolTest, IsSupportedByModel_ServerHintsDoNotGate) {
  TestTool tool;
  // No model declares MATH_ML, so requiring it would filter out every tool.
  auto model = CreateModel(/*supports_tools=*/true,
                           {mojom::ConversationCapability::CHAT});

  EXPECT_TRUE(tool.IsSupportedByModel(
      *model, {mojom::ConversationCapability::CHAT,
               mojom::ConversationCapability::MATH_ML}));
}

}  // namespace ai_chat
