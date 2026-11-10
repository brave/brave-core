// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/test/test_timeouts.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

// What a row of the full page's conversation list shows: the data-testid of
// its indicator in conversations_list/index.tsx, each of which ends in
// "-indicator", or one of the first two values.
constexpr std::string_view kNoRow = "no row";
constexpr std::string_view kNoIndicator = "no indicator";
constexpr std::string_view kGeneratingIndicator = "generating-indicator";
constexpr std::string_view kCompletedIndicator = "completed-indicator";

// A JS function returning what the row of conversation $1 shows, with $2 for
// kNoRow and $3 for kNoIndicator. JsReplace turns `$$` into `$`.
constexpr char kReadRowIndicatorFunction[] = R"(
  () => {
    const row = document.querySelector(
        '[data-testid="conversation-item-' + $1 + '"]')
    if (!row) {
      return $2
    }
    return row.querySelector('[data-testid$$="-indicator"]')?.dataset.testid
        ?? $3
  }
)";

// Called with kReadRowIndicatorFunction: resolves with its result once that is
// $4, checking on each DOM change, or with whatever it is after $5 ms. Rejects
// if it throws, rather than leaving the test to time out.
constexpr char kWaitForRowIndicatorScript[] = R"(
  ((read) => new Promise((resolve, reject) => {
    const settle = () => {
      observer.disconnect()
      clearTimeout(timer)
      try {
        resolve(read())
      } catch (e) {
        reject(e)
      }
    }
    const check = () => {
      try {
        if (read() !== $4) {
          return
        }
      } catch {}
      settle()
    }
    const observer = new MutationObserver(check)
    const timer = setTimeout(settle, $5)
    observer.observe(document, { childList: true, subtree: true })
    check()
  }))
)";

}  // namespace

// Tests what the full page's conversation list shows for a conversation that
// generates in the background, i.e. with no UI connected to it. Its mock
// engine's responses stay in progress until the test releases them.
class AIChatGenerationStatusBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 protected:
  // Opens the full page on a new chat in a new foreground tab and returns its
  // frame. Every other conversation in its list is then a background one.
  content::RenderFrameHost* OpenFullPageOnNewChat() {
    NavigateToConversationUI("");
    return conversation_rfh_;
  }

  // Creates a conversation with a mock engine and, with no UI connected,
  // submits a message to it. Returns the response the engine is asked for,
  // which stays in progress until it is released.
  MockGenerateCallbacks StartBackgroundGeneration() {
    CreateConversationWithMockEngine();
    auto future = SetupMockGenerateAssistantResponse();
    conversation_handler_->SubmitHumanConversationEntry("Background question",
                                                        std::nullopt);
    MockGenerateCallbacks response = future->Take();
    EXPECT_FALSE(conversation_handler_->IsAnyClientConnected());
    return response;
  }

  // Releases a held response with |event| as its only content. After a tool
  // use event the conversation runs the tool and then asks the engine for the
  // next response, so hold that one first with
  // SetupMockGenerateAssistantResponse().
  static void ReleaseResponse(MockGenerateCallbacks response,
                              mojom::ConversationEntryEventPtr event) {
    response.data_callback.Run(
        EngineConsumer::GenerationResultData(std::move(event), std::nullopt));
    std::move(response.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Releases a held response with a text answer, which ends the turn. With no
  // UI connected the service may then unload the conversation, so this drops
  // the pointers to its handler and engine.
  void CompleteResponse(MockGenerateCallbacks response) {
    ReleaseResponse(std::move(response),
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Background answer")));
    conversation_handler_ = nullptr;
    mock_engine_ = nullptr;
  }

  // Waits until the row of conversation |uuid| in |page|'s conversation list
  // shows |expected|, and reports what it shows if that doesn't happen.
  testing::AssertionResult WaitForRowIndicator(
      content::RenderFrameHost* page,
      std::string_view uuid,
      std::string_view expected,
      base::Location location = base::Location::Current()) {
    const std::string script = content::JsReplace(
        base::StrCat(
            {kWaitForRowIndicatorScript, "(", kReadRowIndicatorFunction, ")"}),
        uuid, kNoRow, kNoIndicator, expected,
        static_cast<int>(TestTimeouts::action_timeout().InMilliseconds()));
    auto result = content::EvalJs(page, script, eval_js_options_);
    if (!result.is_ok()) {
      return testing::AssertionFailure()
             << location.ToString() << ": " << result.ExtractError();
    }
    if (result != expected) {
      return testing::AssertionFailure()
             << location.ToString() << ": row shows '" << result.ExtractString()
             << "', expected '" << expected << "'";
    }
    return testing::AssertionSuccess();
  }
};

// A page open before a background conversation starts gets its generation
// state as events: the ring while it generates, then the response ready dot
// once it finishes without having been viewed.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       BackgroundConversationShowsGeneratingThenCompleted) {
  content::RenderFrameHost* page = OpenFullPageOnNewChat();

  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();
  EXPECT_TRUE(WaitForRowIndicator(page, uuid, kGeneratingIndicator));

  CompleteResponse(std::move(response));
  EXPECT_TRUE(WaitForRowIndicator(page, uuid, kCompletedIndicator));
}

// Pages opened during and after generation get the state from the service's
// snapshot, and opening the conversation in one of them clears its dot.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       OpeningConversationClearsCompletedIndicator) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();

  // This page keeps its new chat open throughout, so its row for the
  // conversation only loses the dot if the service clears it.
  content::RenderFrameHost* other_page = OpenFullPageOnNewChat();
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kGeneratingIndicator));

  CompleteResponse(std::move(response));
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kCompletedIndicator));

  content::RenderFrameHost* page = OpenFullPageOnNewChat();
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kCompletedIndicator));

  // Open the conversation from the list, as a click on its row does.
  ASSERT_TRUE(ClickElement(base::StrCat({"conversation-item-", uuid}), page));
  ASSERT_TRUE(base::test::RunUntil([&] {
    ConversationHandler* handler = service_->GetConversation(uuid);
    return handler && handler->IsAnyClientConnected();
  }));

  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
}

}  // namespace ai_chat
