// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/test/run_until.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/hit_test_region_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/point_f.h"

namespace ai_chat {

// Tests for general conversation UI rendering and interactions
class AIChatConversationUIBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  AIChatConversationUIBrowserTest() = default;
  ~AIChatConversationUIBrowserTest() override = default;
};

// Test that when executing a tool, the task UI is not shown for chat
// conversations (it should only be shown for content agent conversations,
// which is verified in the task browser tests).
IN_PROC_BROWSER_TEST_F(
    AIChatConversationUIBrowserTest,
    NonAgentConversationDoesNotShowTaskStateActionsWhileToolExecutes) {
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
  ASSERT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kRunning;
  }));

  // Task UI shouldn't appear
  EXPECT_FALSE(VerifyElementState("task-state-actions", false));
}

// The user-gesture gate is per-pipe rather than per-method, so a suggested
// question click is a sufficient probe for every method on
// UntrustedConversationUserActions.
//
// No script this fixture runs in the untrusted frame before the click under
// test may grant an activation, hence `eval_js_options_`. EvalJs otherwise
// hands the frame a transient activation that outlives the call, which would
// make the no-gesture test pass for the wrong reason, and make the real-click
// test prove nothing.
class AIChatUserGestureBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  AIChatUserGestureBrowserTest() {
    eval_js_options_ = content::EXECUTE_SCRIPT_NO_USER_GESTURE;
  }
  ~AIChatUserGestureBrowserTest() override = default;

 protected:
  static constexpr char kSuggestion[] = "Suggested question";

  // Renders a single suggested question in the untrusted frame.
  void SetUpSuggestion() {
    CreateConversationWithMockEngine();

    // A conversation with no entries has its suggestions replaced with random
    // starter prompts every time associated content is re-evaluated
    // (ConversationHandler::MaybeSeedOrClearSuggestions), which otherwise
    // races the click under test. One completed entry closes that path.
    {
      auto generate_future = SetupMockGenerateAssistantResponse();
      conversation_handler_->SubmitHumanConversationEntry("Opening question",
                                                          std::nullopt);
      auto callbacks = generate_future->Take();
      std::move(callbacks.completed_callback)
          .Run(base::ok(
              EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
    }
    // SubmitSuggestion is a no-op while a request is in flight.
    ASSERT_TRUE(base::test::RunUntil(
        [this] { return !conversation_handler_->IsRequestInProgress(); }));

    // Submitting the suggestion starts a generation of its own. gmock matches
    // the most recently declared expectation first, so this absorbs that call
    // instead of over-saturating the WillOnce above.
    EXPECT_CALL(*mock_engine_, GenerateAssistantResponse)
        .Times(testing::AnyNumber());

    conversation_handler_->SetSuggestedQuestionForTest(kSuggestion,
                                                       "Suggestion prompt");
    NavigateToConversationUI(conversation_handler_->get_conversation_uuid());
    ASSERT_TRUE(VerifyConversationFrameElementText("suggested-question-0",
                                                   kSuggestion));
  }

  content::EvalJsResult EvalInFrame(std::string_view script) {
    return content::EvalJs(GetConversationEntriesFrame(), script,
                           eval_js_options_);
  }

  // |options| decides whether the script itself grants the frame an
  // activation, which is the whole point of these tests.
  bool ClickSuggestion(int options) {
    constexpr char kClickScript[] = R"(
      (function() {
        const el = document.querySelector('[data-testid=suggested-question-0]')
        if (!el) {
          return false
        }
        el.click()
        return true
      })()
    )";
    return content::EvalJs(GetConversationEntriesFrame(), kClickScript, options)
        .ExtractBool();
  }

  bool HasSuggestionEntry() {
    return std::ranges::any_of(
        conversation_handler_->GetConversationHistory(),
        [](const auto& turn) { return turn->text == kSuggestion; });
  }

  bool SuggestionWasSubmitted() {
    return base::test::RunUntil([this] { return HasSuggestionEntry(); });
  }
};

IN_PROC_BROWSER_TEST_F(AIChatUserGestureBrowserTest,
                       UserActionWithoutGestureClosesPipe) {
  SetUpSuggestion();
  ASSERT_EQ(conversation_handler_->GetUserActionReceiverCountForTesting(), 1u);

  ASSERT_TRUE(ClickSuggestion(content::EXECUTE_SCRIPT_NO_USER_GESTURE));

  // The browser rejected the message, which closes the pipe.
  EXPECT_TRUE(base::test::RunUntil([this] {
    return conversation_handler_->GetUserActionReceiverCountForTesting() == 0u;
  }));
  EXPECT_FALSE(HasSuggestionEntry());
}

IN_PROC_BROWSER_TEST_F(AIChatUserGestureBrowserTest,
                       UserActionWithGestureIsAccepted) {
  SetUpSuggestion();

  // EvalJs grants the frame a transient activation unless told not to.
  ASSERT_TRUE(ClickSuggestion(content::EXECUTE_SCRIPT_DEFAULT_OPTIONS));

  EXPECT_TRUE(SuggestionWasSubmitted());
  EXPECT_EQ(conversation_handler_->GetUserActionReceiverCountForTesting(), 1u);
}

// Guards against the gate rejecting genuine input: a false negative here breaks
// the product, which is worse than the hole being open.
IN_PROC_BROWSER_TEST_F(AIChatUserGestureBrowserTest, RealClickIsAccepted) {
  SetUpSuggestion();

  content::RenderFrameHost* frame = GetConversationEntriesFrame();
  content::WaitForHitTestData(frame);

  auto center_result = EvalInFrame(R"(
      (function() {
        const r = document.querySelector('[data-testid=suggested-question-0]')
            .getBoundingClientRect()
        return [r.left + r.width / 2, r.top + r.height / 2]
      })()
    )");
  ASSERT_TRUE(center_result.is_ok()) << center_result.ExtractError();
  const base::ListValue& center = center_result.ExtractList();
  ASSERT_EQ(center.size(), 2u);
  // The untrusted frame is an OOPIF, so its own coordinates need mapping onto
  // the widget the synthetic event is delivered to.
  gfx::PointF point = frame->GetView()->TransformPointToRootCoordSpaceF(
      gfx::PointF(center[0].GetDouble(), center[1].GetDouble()));

  content::SimulateMouseClickAt(
      content::WebContents::FromRenderFrameHost(frame), 0,
      blink::WebMouseEvent::Button::kLeft, gfx::ToRoundedPoint(point));

  EXPECT_TRUE(SuggestionWasSubmitted());
  EXPECT_EQ(conversation_handler_->GetUserActionReceiverCountForTesting(), 1u);
}

}  // namespace ai_chat
