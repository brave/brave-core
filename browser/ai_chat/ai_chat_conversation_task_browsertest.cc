// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "build/build_config.h"
#include "chrome/browser/actor/site_policy.h"
#include "chrome/browser/glic/actor/glic_actor_policy_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/views/tabs/tab_strip_action_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/display_switches.h"

#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ai_chat/content_agent_tool_provider.h"
#include "chrome/browser/actor/actor_keyed_service_factory.h"
#include "components/tabs/public/tab_interface.h"
#include "net/dns/mock_host_resolver.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)

using ::testing::_;
using ::testing::NiceMock;

namespace ai_chat {

// End to end tests for conversations, conversation UI and external side effects
// elsewhere in the browser.

// TODO(https://github.com/brave/brave-browser/issues/51087): Add tests which
// verify the task UI with another tool provider (or a mock tool provider) so we
// don't only verify with the ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE build flag.

#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)

// =============================================================================
// Conversation UI and Tool Use Task State Integration Tests
// =============================================================================
// These tests verify the interaction between ConversationHandler and
// ContentAgentToolProvider specifically, along with the actor framework. They
// verify the complex state interchanges between ai_chat and actor, as well as
// general conversation UI states for tool use.
class AIChatConversationTaskBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  AIChatConversationTaskBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kAIChatAgentProfile);
  }

  ~AIChatConversationTaskBrowserTest() override = default;

  void SetUpOnMainThread() override {
    // Call base class setup first (opts-in to AI Chat)
    AIChatConversationUIBrowserTestBase::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(embedded_https_test_server().Start());

    // Create the agent profile
    base::test::TestFuture<Browser*> browser_future;
    OpenBrowserWindowForAIChatAgentProfileForTesting(
        *browser()->profile(), browser_future.GetCallback());
    Browser* agent_browser = browser_future.Take();
    ASSERT_NE(agent_browser, nullptr);
    agent_profile_ = agent_browser->profile();
    agent_browser_window_ = agent_browser;

    actor::InitActionBlocklist(agent_profile_);

    // Get the AI Chat service from the agent profile (overrides base class)
    service_ = AIChatServiceFactory::GetForBrowserContext(agent_profile_);
    ASSERT_TRUE(service_);

    // Verify content agent is allowed for agent profiles
    EXPECT_TRUE(service_->GetIsContentAgentAllowed());
  }

  void TearDownOnMainThread() override {
    // Clear pointers to conversation-related objects
    GetActorService()->ResetForTesting();
    content_agent_tool_provider_ = nullptr;
    mock_engine_ = nullptr;
    conversation_handler_ = nullptr;
    agent_browser_window_ = nullptr;
    agent_profile_ = nullptr;
    AIChatConversationUIBrowserTestBase::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    AIChatConversationUIBrowserTestBase::SetUpCommandLine(command_line);
    // Ensure physical and css pixels are the same
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "1");
  }

 protected:
  // Creates a conversation with a mock engine in the agent profile
  ConversationHandler* CreateConversationWithMockEngine() {
    AIChatConversationUIBrowserTestBase::CreateConversationWithMockEngine();

    // Get the ContentAgentToolProvider from the conversation.
    // We won't use MockToolProvider since these tests need the real
    // ContentAgentToolProvider to validate it performs its actions.
    auto* tool_provider =
        conversation_handler_->GetFirstToolProviderForTesting();
    EXPECT_TRUE(tool_provider);
    content_agent_tool_provider_ =
        static_cast<ContentAgentToolProvider*>(tool_provider);

    return conversation_handler_;
  }

  // Navigates to the conversation's WebUI in the agent browser
  void NavigateToConversationUI(const std::string& conversation_uuid) {
    AIChatConversationUIBrowserTestBase::NavigateToConversationUI(
        conversation_uuid, agent_browser_window_);
  }

  tabs::TabHandle GetContentAgentTabHandle() {
    return content_agent_tool_provider_->GetTaskTabHandleForTesting();
  }

  actor::ActorKeyedService* GetActorService() {
    return actor::ActorKeyedServiceFactory::GetActorKeyedService(
        agent_profile_);
  }

  actor::ActorTask* GetActorTask() {
    auto task_id = content_agent_tool_provider_->GetTaskId();
    return GetActorService()->GetTask(task_id);
  }

  // Creates a navigate tool use event with JSON arguments
  mojom::ToolUseEventPtr CreateNavigateToolUseEvent(const std::string& tool_id,
                                                    const GURL& url) {
    base::DictValue args;
    args.Set("website_url", url.spec());
    return mojom::ToolUseEvent::New("web_page_navigator", tool_id,
                                    *base::WriteJson(args), std::nullopt,
                                    std::nullopt, nullptr, false);
  }

  raw_ptr<Profile> agent_profile_ = nullptr;
  raw_ptr<Browser> agent_browser_window_ = nullptr;
  raw_ptr<ContentAgentToolProvider> content_agent_tool_provider_ = nullptr;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// This test is consistently failing on MacOS arm64 with cr145. There's already
// an open issue for intermittent failures on both MacOS and Linux:
// https://github.com/brave/brave-browser/issues/51130
#if BUILDFLAG(IS_MAC)
#define MAYBE_TaskPauseResumeActions DISABLED_TaskPauseResumeActions
#else
#define MAYBE_TaskPauseResumeActions TaskPauseResumeActions
#endif
IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest,
                       MAYBE_TaskPauseResumeActions) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  NavigateToConversationUI(uuid);

  // Inject a mock tool so that we can control when the tool execution completes
  // and avoid race conditions between tool completion and pause button clicks.
  auto* mock_tool = static_cast<NiceMock<MockTool>*>(
      content_agent_tool_provider_->AddToolForTesting(
          std::make_unique<NiceMock<MockTool>>("mock_tool", "Mock tool")));

  testing::Sequence tool_call_seq;
  base::OnceClosure tool_execute;

  // Submit first message
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    auto callbacks = generate_future->Take();

    // Set up the mock tool to capture its callback so we control execution
    // timing.
    EXPECT_CALL(*mock_tool, UseTool)
        .InSequence(tool_call_seq)
        .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
          // Store the callback but don't call it yet - we'll call it after
          // clicking pause.
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

  // Wait for task state actions to appear
  EXPECT_TRUE(VerifyElementState("task-state-actions"));

  // Verify the actor task tab is controlled by the agent and disabled for the
  // user.
  EXPECT_TRUE(GetActorTask()->IsUnderActorControl());

  // Verify pause button is visible, resume is not
  EXPECT_TRUE(VerifyElementState("pause-task-button"));
  EXPECT_FALSE(VerifyElementState("resume-task-button", false));
  EXPECT_TRUE(VerifyElementState("stop-task-button"));

  // Use pause button
  ClickElement("pause-task-button");

  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kPaused;
  }));

  // Verify pause and resume switched
  EXPECT_TRUE(VerifyElementState("resume-task-button"));
  EXPECT_FALSE(VerifyElementState("pause-task-button", false));
  EXPECT_TRUE(VerifyElementState("stop-task-button"));

  // Verify the actor task has given control to the user
  EXPECT_TRUE(GetActorTask()->IsUnderUserControl());

  // Now complete the tool execution while we're paused.
  // Tool output should not trigger GenerateAssistantResponse while paused.
  EXPECT_CALL(*mock_engine_, GenerateAssistantResponse)
      .Times(0)
      .InSequence(tool_call_seq);
  std::move(tool_execute).Run();

  // Handle the tool execution response after resuming
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);

    // Use the resume button
    ClickElement("resume-task-button");

    auto callbacks = generate_future->Take();

    EXPECT_EQ(GetConversationState()->tool_use_task_state,
              mojom::TaskState::kRunning);

    // Verify the actor task tab is controlled by the agent and disabled for the
    // user.
    EXPECT_TRUE(GetActorTask()->IsUnderActorControl());

    // Simulate no more tool use requests, which should trigger task
    // completion.
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("all done")),
        std::nullopt));
    // Complete successful response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kNone;
  }));

  // Task state buttons should not exist anymore
  EXPECT_FALSE(VerifyElementState("task-state-actions", false));
}

IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest, TaskStopAction) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  NavigateToConversationUI(uuid);

  // Submit first message
  {
    auto generate_future = SetupMockGenerateAssistantResponse();
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    auto callbacks = generate_future->Take();
    // Send first message response
    // Simulate tool use event
    GURL test_url = embedded_https_test_server().GetURL("/actor/link.html");
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_1", test_url)),
        std::nullopt));
    // Complete first message response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Handle the tool execution response. We're letting the task complete one
  // round of tool execution so we can have ContentAgentToolProvider perform
  // an action in a tab and add it to the actor Task. We want to verify that
  // stopping the task does not cause any interaction issues with
  // ConversationHandler.
  {
    auto generate_future = SetupMockGenerateAssistantResponse();

    EXPECT_EQ(GetConversationState()->tool_use_task_state,
              mojom::TaskState::kRunning);

    auto callbacks = generate_future->Take();

    // If the tool output is sent, we can verify that the tool performed
    // its action successfully.
    EXPECT_EQ(GetContentAgentTabHandle()
                  .Get()
                  ->GetContents()
                  ->GetLastCommittedURL()
                  .path(),
              "/actor/link.html");

    // Use stop button here to show that stopping a task does not stop the
    // assistant response requests - it only stops executing any tool use
    // requests.
    ASSERT_TRUE(VerifyElementState("task-state-actions"));
    ASSERT_TRUE(VerifyElementState("stop-task-button"));
    ClickElement("stop-task-button");

    GURL test_url = embedded_https_test_server().GetURL("/actor/drag.html");
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Hmm, I want a different page")),
        std::nullopt));
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_2", test_url)),
        std::nullopt));
    // Complete successful response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Second tool output should not be sent because we are going to stop
  EXPECT_CALL(*mock_engine_, GenerateAssistantResponse).Times(0);

  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kStopped;
  }));

  // Task state buttons should not exist anymore
  EXPECT_FALSE(VerifyElementState("task-state-actions", false));

  // After stopping, verify the tab is no longer controlled by a task
  EXPECT_EQ(nullptr, GetActorService()->GetTaskFromTab(
                         *GetContentAgentTabHandle().Get()))
      << "Actor task state: "
      << GetActorService()
             ->GetTaskFromTab(*GetContentAgentTabHandle().Get())
             ->GetState();

  // After stopping, submit a new human message to verify that the task can be
  // re-started with new state.
  {
    auto generate_future = SetupMockGenerateAssistantResponse();
    conversation_handler_->SubmitHumanConversationEntry(
        "Actually do something different", std::nullopt);

    EXPECT_TRUE(base::test::RunUntil([this]() {
      return GetConversationState()->tool_use_task_state ==
             mojom::TaskState::kNone;
    }));

    auto callbacks = generate_future->Take();
    // Send first message response
    // Simulate tool use event
    GURL test_url = embedded_https_test_server().GetURL("/actor/link.html");
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_1", test_url)),
        std::nullopt));
    // Complete first message response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kRunning;
  }));

  // The tool will execute and response sent - we need to set up a new
  // expectation so the previous one is not triggered.
  {
    auto generate_future = SetupMockGenerateAssistantResponse();
    std::ignore = generate_future->Take();
  }
}

IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest, TaskUI) {
  // The task UI shows for any assistant response group containing at least
  // one task tool use; the ProgressBubble surfaces the active tool label or
  // a "Thinking" placeholder while the loop runs, and a completion/paused/
  // stopped state once the loop ends.
  const std::string thinking_label =
      l10n_util::GetStringUTF8(IDS_CHAT_UI_TOOL_LABEL_THINKING);
  const std::string paused_label =
      l10n_util::GetStringUTF8(IDS_CHAT_UI_TASK_STATE_PAUSED_LABEL);
  const std::string complete_label =
      l10n_util::GetStringUTF8(IDS_CHAT_UI_TOOL_LABEL_COMPLETE);

  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  NavigateToConversationUI(uuid);

  // Inject our own Tool so that we can handle the tool execution and pause
  auto* mock_tool = static_cast<NiceMock<MockTool>*>(
      content_agent_tool_provider_->AddToolForTesting(
          std::make_unique<NiceMock<MockTool>>("mock_tool", "Mock tool")));

  testing::Sequence tool_call_seq;
  base::OnceClosure tool_execute;
  // Submit first message
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    auto callbacks = generate_future->Take();
    // Send first message response
    // Simulate tool use event
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateToolUseEvent("mock_tool", "tool_id_1")),
        std::nullopt));

    EXPECT_CALL(*mock_tool, UseTool)
        .InSequence(tool_call_seq)
        .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
          // Wait unt the next round is setup to call the callback
          tool_execute = base::BindOnce(
              [](Tool::UseToolCallback callback) {
                std::move(callback).Run(
                    CreateContentBlocksForText("1st tool result"), {});
              },
              std::move(callback));
        }));

    // Complete first message response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // The task UI shows as soon as there's at least one task tool use, and
  // the progress bubble displays the tool label while the tool runs.
  EXPECT_TRUE(VerifyConversationFrameElementState("assistant-task"));
  EXPECT_TRUE(VerifyConversationFrameElementText("progress-bubble-description",
                                                 "mock_tool"));
  // Handle the tool execution response with another tool use request.
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);
    std::move(tool_execute).Run();
    auto callbacks = generate_future->Take();

    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Hmm, I want a different thing")),
        std::nullopt));
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateToolUseEvent("mock_tool", "tool_id_2")),
        std::nullopt));

    // When the tool is being executed, we can verify the UI state
    EXPECT_CALL(*mock_tool, UseTool)
        .InSequence(tool_call_seq)
        .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
          EXPECT_TRUE(VerifyConversationFrameElementState("assistant-task"));
          // While a tool is executing the bubble shows that tool's label,
          // not the "Thinking" placeholder.
          EXPECT_TRUE(VerifyConversationFrameElementText(
              "progress-bubble-description", "mock_tool"));
          tool_execute = base::BindOnce(
              [](Tool::UseToolCallback callback) {
                std::move(callback).Run(
                    CreateContentBlocksForText("2nd tool result"), {});
              },
              std::move(callback));
        }));

    // Complete successful response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Handle the second tool execution response with pausing and verify UI label.
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);

    // Finish executing the tool
    std::move(tool_execute).Run();

    auto callbacks = generate_future->Take();

    // Now we should be thinking - tool execution finished, waiting for the
    // engine to respond.
    EXPECT_TRUE(VerifyConversationFrameElementText(
        "progress-bubble-description", thinking_label));

    // Shouldn't call the tool again because we are pausing.
    EXPECT_CALL(*mock_tool, UseTool).Times(0).InSequence(tool_call_seq);

    // Pause the task directly via C++ to avoid a timing race with the UI
    // click handler. The SvelteToReact wrapper in @brave/leo attaches click
    // listeners asynchronously via useEffect, so the listener may not be
    // attached when ClickElement fires. The UI click path is tested by
    // TaskPauseResumeActions.
    // TODO(https://github.com/brave/leo/issues/1343): Remove this workaround
    // once SvelteToReact attaches listeners synchronously.
    conversation_handler_->PauseTask();
    EXPECT_EQ(GetConversationState()->tool_use_task_state,
              mojom::TaskState::kPaused);

    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Hmm, I want a different thing")),
        std::nullopt));
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateToolUseEvent("mock_tool", "tool_id_3")),
        std::nullopt));
    // Complete successful response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // The task's progress bubble should now read "Paused"
  EXPECT_TRUE(VerifyConversationFrameElementText("progress-bubble-description",
                                                 paused_label));

  // When we submit a new message, the task is no longer active. It should still
  // exist but should not have its "paused" label.
  {
    auto generate_future = SetupMockGenerateAssistantResponse(&tool_call_seq);
    conversation_handler_->SubmitHumanConversationEntry(
        "Actually do something different", std::nullopt);

    EXPECT_TRUE(base::test::RunUntil([this]() {
      return GetConversationState()->tool_use_task_state ==
             mojom::TaskState::kNone;
    }));

    auto callbacks = generate_future->Take();
    // Simple response
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("ok")),
        std::nullopt));
    // Complete first message response
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }
  EXPECT_TRUE(VerifyConversationFrameElementState("assistant-task"));
  // The prior, no-longer-active task settles into the completed state — its
  // bubble no longer reads "Paused".
  EXPECT_TRUE(VerifyConversationFrameElementText("progress-bubble-description",
                                                 complete_label));
}

// Verify that the upstream GlicAndActorButtonsContainer is not constructed in
// either the toolbar or the tab strip of a window that is executing a task.
// The container has been the source of crashes; Brave disables it by
// overriding the `kGlicActorUiTaskIcon` feature param to false (see
// chromium_src/chrome/common/chrome_features.cc). This test guards against
// regressions of that override.
IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest,
                       NoUpstreamGlicAndActorButtonsContainer) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();
  NavigateToConversationUI(uuid);

  // Drive the conversation into a running task state via a tool use event.
  {
    auto generate_future = SetupMockGenerateAssistantResponse();
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    auto callbacks = generate_future->Take();
    GURL test_url = embedded_https_test_server().GetURL("/actor/link.html");
    callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_1", test_url)),
        std::nullopt));
    std::move(callbacks.completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }
  ASSERT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kRunning;
  }));

  // The toolbar must not contain the upstream glic actor task icon.
  BrowserView* agent_browser_view =
      BrowserView::GetBrowserViewForBrowser(agent_browser_window_);
  ASSERT_TRUE(agent_browser_view);
  EXPECT_EQ(agent_browser_view->toolbar()->glic_actor_task_icon(), nullptr);

  // The tab strip's action container must not contain the upstream glic actor
  // button container. The container itself may be absent depending on the
  // window's tab strip configuration; if so, there is nothing to verify.
  auto* tab_strip_action_container =
      BrowserElementsViews::From(agent_browser_window_)
          ->GetViewAs<TabStripActionContainer>(
              kTabStripActionContainerElementId);
  if (tab_strip_action_container) {
    EXPECT_EQ(tab_strip_action_container->glic_actor_button_container(),
              nullptr);
  }
}

#endif  // BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)

}  // namespace ai_chat
