// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::NiceMock;

namespace ai_chat {

// Tests for the conversation entries UI component rendering and interactions.
class AIChatConversationEntriesBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  AIChatConversationEntriesBrowserTest() = default;
  ~AIChatConversationEntriesBrowserTest() override = default;
};

// Test that conversation entries are properly rendered when reloading an
// existing conversation, and that the edit button is available in the context
// menu. This is a regression test for an issue where canSubmitUserEntries was
// not properly initialized when loading an existing conversation, causing the
// edit button to not appear.
IN_PROC_BROWSER_TEST_F(AIChatConversationEntriesBrowserTest,
                       ConversationEntriesRenderedAfterReload) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  // Navigate to the conversation UI first
  NavigateToConversationUI(uuid);

  // Set up the mock engine to handle the message submission
  EngineConsumer::GenerationDataCallback data_callback;
  EngineConsumer::GenerationCompletedCallback completed_callback;

  base::RunLoop wait_for_generate;
  EXPECT_CALL(*mock_engine_, GenerateAssistantResponse(_, _, _, _, _, _, _, _))
      .WillOnce([&](PageContentsMap page_contents,
                    const EngineConsumer::ConversationHistory& history,
                    bool is_temporary,
                    const std::vector<base::WeakPtr<Tool>>& provided_tools,
                    std::optional<std::string_view> preferred_tool_name,
                    const ConversationCapabilitySet& capabilities,
                    EngineConsumer::GenerationDataCallback data_cb,
                    EngineConsumer::GenerationCompletedCallback complete_cb) {
        data_callback = std::move(data_cb);
        completed_callback = std::move(complete_cb);
        wait_for_generate.Quit();
      });

  // Submit a message to create a human entry
  conversation_handler_->SubmitHumanConversationEntry("Test message",
                                                      std::nullopt);
  wait_for_generate.Run();

  ASSERT_TRUE(VerifyConversationFrameElementState("human-turn"))
      << "Human turn element not found after submitting message";

  // Complete the assistant response
  data_callback.Run(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("Test response")),
      std::nullopt));
  std::move(completed_callback)
      .Run(base::ok(
          EngineConsumer::GenerationResultData(nullptr, std::nullopt)));

  // Wait for the response to be complete
  EXPECT_TRUE(base::test::RunUntil(
      [this]() { return !GetConversationState()->is_request_in_progress; }));

  ASSERT_TRUE(VerifyConversationFrameElementState("assistant-turn"))
      << "Assistant turn element not found after response";

  // Reload the conversation in a new tab to verify state is the same
  // when present at initial load.
  browser()->tab_strip_model()->CloseWebContentsAt(
      browser()->tab_strip_model()->active_index(), TabCloseTypes::CLOSE_NONE);
  NavigateToConversationUI(uuid);

  EXPECT_TRUE(VerifyConversationFrameElementState("human-turn"))
      << "Human turn element not found after reloading conversation";

  EXPECT_TRUE(VerifyConversationFrameElementState("assistant-turn"))
      << "Assistant turn element not found after reloading conversation";

  // The key regression test: verify the edit button appears in the context menu
  // after reload. Before the fix, canSubmitUserEntries wasn't properly
  // initialized when loading an existing conversation, so the edit button
  // wouldn't appear.
  ASSERT_TRUE(
      VerifyConversationFrameElementState("context-menu-human-open-button"))
      << "Context menu button not found on human turn";

  ASSERT_TRUE(ClickElement("context-menu-human-open-button",
                           GetConversationEntriesFrame()))
      << "Failed to click context menu button to open menu";

  EXPECT_TRUE(VerifyConversationFrameElementState("edit-question-button"))
      << "Edit question button not found after reloading conversation - "
         "canSubmitUserEntries may not be properly initialized";
}

}  // namespace ai_chat
