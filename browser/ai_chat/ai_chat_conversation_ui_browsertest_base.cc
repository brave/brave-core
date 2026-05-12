// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"

using ::testing::_;
using ::testing::NiceMock;

namespace ai_chat {

MockGenerateCallbacks::MockGenerateCallbacks() = default;
MockGenerateCallbacks::MockGenerateCallbacks(
    EngineConsumer::GenerationDataCallback data_cb,
    EngineConsumer::GenerationCompletedCallback complete_cb)
    : data_callback(std::move(data_cb)),
      completed_callback(std::move(complete_cb)) {}
MockGenerateCallbacks::~MockGenerateCallbacks() = default;
MockGenerateCallbacks::MockGenerateCallbacks(MockGenerateCallbacks&&) = default;
MockGenerateCallbacks& MockGenerateCallbacks::operator=(
    MockGenerateCallbacks&&) = default;

void AIChatConversationUIBrowserTestBase::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();
  // Opt-in to AI Chat
  SetUserOptedIn(browser()->profile()->GetPrefs(), true);
  service_ = AIChatServiceFactory::GetForBrowserContext(browser()->profile());
  ASSERT_TRUE(service_);
}

void AIChatConversationUIBrowserTestBase::TearDownOnMainThread() {
  mock_engine_ = nullptr;
  conversation_handler_ = nullptr;
  conversation_rfh_ = nullptr;
  service_ = nullptr;
  InProcessBrowserTest::TearDownOnMainThread();
}

void AIChatConversationUIBrowserTestBase::NavigateToConversationUI(
    const std::string& conversation_uuid,
    Browser* target_browser) {
  if (!target_browser) {
    target_browser = browser();
  }
  // Reset before navigation to avoid dangling pointer if previous frame was
  // destroyed (e.g., tab was closed).
  conversation_rfh_ = nullptr;
  GURL url(base::StrCat({"chrome://leo-ai/", conversation_uuid}));
  conversation_rfh_ = ui_test_utils::NavigateToURLWithDisposition(
      target_browser, url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  // Wait for child frame to exist
  VerifyElementState("conversation-entries-iframe");
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return GetConversationEntriesFrame() != nullptr; }));
}

content::RenderFrameHost*
AIChatConversationUIBrowserTestBase::GetConversationEntriesFrame() {
  content::RenderFrameHost* result = nullptr;
  conversation_rfh_->ForEachRenderFrameHost(
      [&](content::RenderFrameHost* frame) {
        if (frame->GetWebUI() && frame->GetWebUI()
                                     ->GetController()
                                     ->GetAs<AIChatUntrustedConversationUI>()) {
          result = frame;
          return;
        }
      });
  return result;
}

bool AIChatConversationUIBrowserTestBase::VerifyConversationFrameElementState(
    const std::string& test_id,
    bool expect_exist,
    base::Location location) {
  return VerifyElementState(test_id, expect_exist,
                            GetConversationEntriesFrame(), location);
}

bool AIChatConversationUIBrowserTestBase::VerifyElementState(
    const std::string& test_id,
    bool expect_exist,
    content::RenderFrameHost* frame,
    base::Location location) {
  if (!frame) {
    frame = conversation_rfh_;
  }
  SCOPED_TRACE(testing::Message()
               << "VerifyElementState: '" << test_id << "' called from "
               << location.file_name() << ":" << location.line_number());
  constexpr char kWaitForAIChatRenderScript[] = R"(
    new Promise((resolve, reject) => {
      const selector = `[data-testid=$1]`
      const expectsNotExist = $2
      const timeoutMs = 10000

      function checkElement() {
        let element = document.querySelector(selector)

        if (element && !expectsNotExist) {
          return true
        }
        if (!element && expectsNotExist) {
          return true
        }
        return false
      }

      if (checkElement()) {
        resolve(!expectsNotExist)
        return
      }

      const startTime = Date.now()
      const observeTarget = document.body || document.documentElement

      if (!observeTarget) {
        // Document not ready, use polling fallback
        const interval = setInterval(() => {
          if (checkElement()) {
            clearInterval(interval)
            resolve(!expectsNotExist)
          } else if (Date.now() - startTime > timeoutMs) {
            clearInterval(interval)
            resolve(expectsNotExist)
          }
        }, 100)
        return
      }

      const observer = new MutationObserver(() => {
        if (checkElement()) {
          observer.disconnect()
          resolve(!expectsNotExist)
        } else if (Date.now() - startTime > timeoutMs) {
          observer.disconnect()
          resolve(expectsNotExist)
        }
      })
      observer.observe(observeTarget, { childList: true, subtree: true })

      // Fallback timeout in case no mutations happen
      setTimeout(() => {
        observer.disconnect()
        resolve(expectsNotExist)
      }, timeoutMs)
    })
  )";

  auto result = content::EvalJs(
      frame,
      content::JsReplace(kWaitForAIChatRenderScript, test_id, !expect_exist));
  return result.ExtractBool();
}

bool AIChatConversationUIBrowserTestBase::ClickElement(
    const std::string& test_id,
    content::RenderFrameHost* frame) {
  if (!frame) {
    frame = conversation_rfh_;
  }
  constexpr char kClickElementScript[] = R"(
    (function() {
      const el = document.querySelector('[data-testid=$1]')
      if (el) {
        el.click()
        return true
      }
      return false
    })()
  )";
  return content::EvalJs(frame,
                         content::JsReplace(kClickElementScript, test_id))
      .ExtractBool();
}

bool AIChatConversationUIBrowserTestBase::HoverElement(
    const std::string& test_id,
    content::RenderFrameHost* frame) {
  if (!frame) {
    frame = conversation_rfh_;
  }
  constexpr char kHoverElementScript[] = R"(
    (function() {
      const el = document.querySelector('[data-testid=$1]')
      if (el) {
        el.dispatchEvent(new MouseEvent('mouseenter', {
          bubbles: true,
          cancelable: true,
          view: window
        }))
        return true
      }
      return false
    })()
  )";
  return content::EvalJs(frame,
                         content::JsReplace(kHoverElementScript, test_id))
      .ExtractBool();
}

ConversationHandler*
AIChatConversationUIBrowserTestBase::CreateConversationWithMockEngine() {
  conversation_handler_ = service_->CreateConversation();
  EXPECT_TRUE(conversation_handler_);

  // Inject mock engine
  auto mock_engine = std::make_unique<NiceMock<MockEngineConsumer>>();
  mock_engine_ = mock_engine.get();
  conversation_handler_->SetEngineForTesting(std::move(mock_engine));

  return conversation_handler_;
}

mojom::ConversationStatePtr
AIChatConversationUIBrowserTestBase::GetConversationState() {
  base::test::TestFuture<mojom::ConversationStatePtr> future;
  conversation_handler_->GetState(future.GetCallback());
  return future.Take();
}

std::unique_ptr<base::test::TestFuture<MockGenerateCallbacks>>
AIChatConversationUIBrowserTestBase::SetupMockGenerateAssistantResponse(
    testing::Sequence* sequence) {
  auto future =
      std::make_unique<base::test::TestFuture<MockGenerateCallbacks>>();
  auto* future_ptr = future.get();
  auto& expectation = EXPECT_CALL(
      *mock_engine_, GenerateAssistantResponse(_, _, _, _, _, _, _, _));
  if (sequence) {
    expectation.InSequence(*sequence);
  }
  expectation.WillOnce(
      [future_ptr](PageContentsMap page_contents,
                   const EngineConsumer::ConversationHistory& history,
                   bool is_temporary,
                   const std::vector<base::WeakPtr<Tool>>& provided_tools,
                   std::optional<std::string_view> preferred_tool_name,
                   const ConversationCapabilitySet& capabilities,
                   EngineConsumer::GenerationDataCallback data_cb,
                   EngineConsumer::GenerationCompletedCallback complete_cb) {
        future_ptr->SetValue(
            MockGenerateCallbacks(std::move(data_cb), std::move(complete_cb)));
      });
  return future;
}

}  // namespace ai_chat
