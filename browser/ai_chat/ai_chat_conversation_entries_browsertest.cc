// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/path_service.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/threading/thread_restrictions.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

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
  auto generate_future = SetupMockGenerateAssistantResponse();

  // Submit a message to create a human entry
  conversation_handler_->SubmitHumanConversationEntry("Test message",
                                                      std::nullopt);
  auto callbacks = generate_future->Take();

  ASSERT_TRUE(VerifyConversationFrameElementState("human-turn"))
      << "Human turn element not found after submitting message";

  // Complete the assistant response
  callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("Test response")),
      std::nullopt));
  std::move(callbacks.completed_callback)
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

// The image sources the conversation entries frame loads from must allow it, as
// WebUIURLLoaderFactory rejects cross-origin loads between chrome-untrusted://
// hosts otherwise.
IN_PROC_BROWSER_TEST_F(AIChatConversationEntriesBrowserTest,
                       LoadsUntrustedImages) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    embedded_https_test_server().ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
  }
  ASSERT_TRUE(embedded_https_test_server().Start());

  CreateConversationWithMockEngine();
  NavigateToConversationUI(conversation_handler_->get_conversation_uuid());
  content::RenderFrameHost* frame = GetConversationEntriesFrame();
  ASSERT_TRUE(frame);

  constexpr char kLoadImageScript[] = R"(
    new Promise(resolve => {
      const img = new Image();
      img.onload = () => resolve('loaded');
      img.onerror = () => resolve('error');
      img.src = $1;
    });
  )";
  const GURL image_urls[] = {
      GURL(base::StrCat(
          {"chrome-untrusted://image?url=",
           base::EscapeQueryParamValue(
               embedded_https_test_server().GetURL("/logo.png").spec(),
               /*use_plus=*/false)})),
      GURL("chrome-untrusted://favicon2?size=64&pageUrl=https%3A%2F%2Fa.com"),
  };
  for (const GURL& image_url : image_urls) {
    SCOPED_TRACE(image_url);
    EXPECT_EQ("loaded",
              content::EvalJs(frame,
                              content::JsReplace(kLoadImageScript, image_url)));
  }
}

}  // namespace ai_chat
