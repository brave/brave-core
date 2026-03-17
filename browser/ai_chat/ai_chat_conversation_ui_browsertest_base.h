// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_CONVERSATION_UI_BROWSERTEST_BASE_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_CONVERSATION_UI_BROWSERTEST_BASE_H_

#include <memory>
#include <string>

#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/render_frame_host.h"

class Browser;

namespace ai_chat {

// Holds the callbacks captured from a mocked GenerateAssistantResponse call.
struct MockGenerateCallbacks {
  MockGenerateCallbacks();
  MockGenerateCallbacks(
      EngineConsumer::GenerationDataCallback data_cb,
      EngineConsumer::GenerationCompletedCallback complete_cb);
  ~MockGenerateCallbacks();
  MockGenerateCallbacks(MockGenerateCallbacks&&);
  MockGenerateCallbacks& operator=(MockGenerateCallbacks&&);

  EngineConsumer::GenerationDataCallback data_callback;
  EngineConsumer::GenerationCompletedCallback completed_callback;
};

// Base class for AI Chat conversation UI browser tests.
// Provides common utilities for navigating to conversation UI, accessing
// conversation frames, and verifying element state in the UI.
class AIChatConversationUIBrowserTestBase : public InProcessBrowserTest {
 public:
  AIChatConversationUIBrowserTestBase() = default;
  ~AIChatConversationUIBrowserTestBase() override = default;

  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

 protected:
  // Navigates to the conversation's WebUI
  void NavigateToConversationUI(const std::string& conversation_uuid,
                                Browser* target_browser = nullptr);

  // Gets the conversation entries iframe render frame host
  content::RenderFrameHost* GetConversationEntriesFrame();

  // Helper to check if an element with a specific data-testid exists in the
  // conversation entries frame
  bool VerifyConversationFrameElementState(
      const std::string& test_id,
      bool expect_exist = true,
      base::Location location = base::Location::Current());

  // Helper to check if an element with a specific data-testid exists.
  // Waits up to 10 seconds for the element to appear/disappear based on
  // expect_exist.
  bool VerifyElementState(const std::string& test_id,
                          bool expect_exist = true,
                          content::RenderFrameHost* frame = nullptr,
                          base::Location location = base::Location::Current());

  // Helper to click an element with a specific data-testid
  bool ClickElement(const std::string& test_id,
                    content::RenderFrameHost* frame = nullptr);

  // Helper to simulate hovering over an element with a specific data-testid.
  // Dispatches mouseenter event to trigger React's onMouseEnter handler.
  bool HoverElement(const std::string& test_id,
                    content::RenderFrameHost* frame = nullptr);

  // Creates a conversation with a mock engine for testing
  ConversationHandler* CreateConversationWithMockEngine();

  // Gets the current conversation state
  mojom::ConversationStatePtr GetConversationState();

  // Sets up mock expectation for GenerateAssistantResponse and returns a future
  // that resolves when the mock is called. Call future->Take() to block and get
  // the captured callbacks. Optionally pass a sequence for ordered
  // expectations.
  std::unique_ptr<base::test::TestFuture<MockGenerateCallbacks>>
  SetupMockGenerateAssistantResponse(testing::Sequence* sequence = nullptr);

  raw_ptr<content::RenderFrameHost> conversation_rfh_ = nullptr;
  raw_ptr<AIChatService> service_ = nullptr;
  raw_ptr<ConversationHandler> conversation_handler_ = nullptr;
  raw_ptr<MockEngineConsumer> mock_engine_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_CONVERSATION_UI_BROWSERTEST_BASE_H_
