// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/test/run_until.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

// Tests for general conversation UI rendering and interactions
class AIChatConversationUIBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  AIChatConversationUIBrowserTest() = default;
  ~AIChatConversationUIBrowserTest() override = default;
};

// Test that when executing a tool, the task UI is not shown for chat
// conversations (should only be shown for content agent conversations, verified
// in the task browser tests.
IN_PROC_BROWSER_TEST_F(AIChatConversationUIBrowserTest, NoTaskStateActions) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  NavigateToConversationUI(uuid);

  // Inject a mock tool provider with a mock tool so that we can control when
  // the tool execution completes and inspect the UI.
  auto* mock_tool_provider = conversation_handler_->AddToolProviderForTesting(
      std::make_unique<testing::NiceMock<MockToolProvider>>());
  auto* mock_tool = mock_tool_provider->AddToolForTesting(
      std::make_unique<testing::NiceMock<MockTool>>("mock_tool", "Mock tool"));

  testing::Sequence tool_call_seq;
  base::OnceClosure tool_execute;

  // Submit first message
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);
    conversation_handler_->SubmitHumanConversationEntry("Do something",
                                                        std::nullopt);
    auto callbacks = generate_future->Take();

    // Set up the mock tool to capture its callback so we control execution
    // timing.
    EXPECT_CALL(*mock_tool, UseTool)
        .InSequence(tool_call_seq)
        .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
          // Store the callback but don't call it yet so we can inspect the
          // state whilst waiting for the tool execution to complete.
          tool_execute = base::BindOnce(
              [](Tool::UseToolCallback callback) {
                std::move(callback).Run(
                    CreateContentBlocksForText("tool result"), {});
              },
              std::move(callback));
        }));

    // Simulate tool use event
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateToolUseEvent("mock_tool", "tool_id_1")),
        std::nullopt));
    // Complete first message response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Wait for running state
  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kRunning;
  }));

  // Task UI shouldn't appear
  EXPECT_FALSE(VerifyElementState("task-state-actions", false));
}

}  // namespace ai_chat
