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
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
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
#include "chrome/browser/actor/actor_policy_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
#include "url/url_constants.h"

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
// TODO(https://github.com/brave/brave-browser/issues/51087): extract common
// setup and utility functions to a base class.
class AIChatConversationTaskBrowserTest : public InProcessBrowserTest {
 public:
  AIChatConversationTaskBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kAIChatAgentProfile);
  }

  ~AIChatConversationTaskBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(embedded_https_test_server().Start());

    // Opt-in to AI Chat in the regular profile first
    auto* profile = browser()->profile();
    SetUserOptedIn(profile->GetPrefs(), true);

    // Create the agent profile
    base::test::TestFuture<Browser*> browser_future;
    OpenBrowserWindowForAIChatAgentProfileForTesting(
        *profile, browser_future.GetCallback());
    Browser* agent_browser = browser_future.Take();
    ASSERT_NE(agent_browser, nullptr);
    agent_profile_ = agent_browser->profile();
    agent_browser_window_ = agent_browser;

    GetActorService()->GetPolicyChecker().SetActOnWebForTesting(true);
    actor::InitActionBlocklist(agent_profile_);

    // Get the AI Chat service from the agent profile
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
    service_ = nullptr;
    agent_browser_window_ = nullptr;
    conversation_rfh_ = nullptr;
    agent_profile_ = nullptr;
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // Ensure physical and css pixels are the same
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "1");
  }

 protected:
  // Creates a conversation with a mock engine in the agent profile
  ConversationHandler* CreateConversationWithMockEngine() {
    conversation_handler_ = service_->CreateConversation();
    EXPECT_TRUE(conversation_handler_);

    // Inject mock engine
    auto mock_engine = std::make_unique<NiceMock<MockEngineConsumer>>();
    mock_engine_ = mock_engine.get();
    conversation_handler_->SetEngineForTesting(std::move(mock_engine));

    // Get the ContentAgentToolProvider from the conversation
    auto* tool_provider =
        conversation_handler_->GetFirstToolProviderForTesting();
    EXPECT_TRUE(tool_provider);
    content_agent_tool_provider_ =
        static_cast<ContentAgentToolProvider*>(tool_provider);

    return conversation_handler_;
  }

  // Navigates to the conversation's WebUI in the agent browser
  void NavigateToConversationUI(const std::string& conversation_uuid) {
    GURL url(base::StrCat({"chrome://leo-ai/", conversation_uuid}));
    conversation_rfh_ = ui_test_utils::NavigateToURLWithDisposition(
        agent_browser_window_, url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    // Wait for child frame to exist
    VerifyElementState("conversation-entries-iframe");
    EXPECT_TRUE(base::test::RunUntil(
        [&] { return GetConversationEntriesFrame() != nullptr; }));
  }

  content::RenderFrameHost* GetConversationEntriesFrame() {
    content::RenderFrameHost* result = nullptr;
    conversation_rfh_->ForEachRenderFrameHost(
        [&](content::RenderFrameHost* frame) {
          if (frame->GetWebUI() &&
              frame->GetWebUI()
                  ->GetController()
                  ->GetAs<AIChatUntrustedConversationUI>()) {
            result = frame;
            return;
          }
        });
    return result;
  }

  bool VerifyConversationFrameElementState(
      const std::string& test_id,
      bool expect_exist = true,
      base::Location location = base::Location::Current()) {
    return VerifyElementState(test_id, expect_exist,
                              GetConversationEntriesFrame(), location);
  }

  // Helper to check if an element with a specific data-testid exists
  bool VerifyElementState(const std::string& test_id,
                          bool expect_exist = true,
                          content::RenderFrameHost* frame = nullptr,
                          base::Location location = base::Location::Current()) {
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

        function checkElement() {
          let element = document.querySelector(selector)

          if (element && !expectsNotExist) {
            resolve(true)
            return
          }
          if (!element && expectsNotExist) {
            resolve(false)
            return
          }
        }

        checkElement()

        const observer = new MutationObserver(() => {
          checkElement()
        })
        observer.observe(document.documentElement,
            { childList: true, subtree: true })
      })
    )";

    auto result = content::EvalJs(
        frame,
        content::JsReplace(kWaitForAIChatRenderScript, test_id, !expect_exist));
    return result.ExtractBool();
  }

  // Helper to click an element with a specific data-testid
  bool ClickElement(const std::string& test_id) {
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
    return content::EvalJs(conversation_rfh_,
                           content::JsReplace(kClickElementScript, test_id))
        .ExtractBool();
  }

  // Gets the current conversation state
  mojom::ConversationStatePtr GetConversationState() {
    base::test::TestFuture<mojom::ConversationStatePtr> future;
    conversation_handler_->GetState(future.GetCallback());
    return future.Take();
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

  // Sets up the mock engine to capture callbacks for tool use simulation
  base::OnceClosure SetupMockGenerateAssistantResponse(
      EngineConsumer::GenerationDataCallback* out_data_callback,
      EngineConsumer::GenerationCompletedCallback* out_completed_callback,
      testing::Sequence* sequence = nullptr,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    auto run_loop = std::make_unique<base::RunLoop>();
    auto on_generate_called = run_loop->QuitClosure();
    auto& expect =
        EXPECT_CALL(*mock_engine_,
                    GenerateAssistantResponse(_, _, _, _, _, _, _, _, _))
            .Description(base::StrCat({"GenerateAssistantResponse mocked from ",
                                       location.ToString()}));
    if (sequence) {
      expect.InSequence(*sequence);
    }
    expect.WillOnce(
        [out_data_callback, out_completed_callback,
         on_called = std::move(on_generate_called)](
            PageContentsMap page_contents,
            const EngineConsumer::ConversationHistory& history,
            const std::string& selected_language, bool is_temporary,
            const std::vector<base::WeakPtr<Tool>>& provided_tools,
            std::optional<std::string_view> preferred_tool_name,
            mojom::ConversationCapability capability,
            EngineConsumer::GenerationDataCallback data_cb,
            EngineConsumer::GenerationCompletedCallback complete_cb) mutable {
          *out_data_callback = std::move(data_cb);
          *out_completed_callback = std::move(complete_cb);
          std::move(on_called).Run();
        });
    return base::BindOnce(
        [](std::unique_ptr<base::RunLoop> run_loop) { run_loop->Run(); },
        std::move(run_loop));
  }

  // Creates a navigate tool use event with JSON arguments
  mojom::ToolUseEventPtr CreateNavigateToolUseEvent(const std::string& tool_id,
                                                    const GURL& url) {
    base::Value::Dict args;
    args.Set("website_url", url.spec());
    return mojom::ToolUseEvent::New("web_page_navigator", tool_id,
                                    *base::WriteJson(args), std::nullopt,
                                    nullptr);
  }

  mojom::ToolUseEventPtr CreateToolUseEvent(const std::string& tool_name,
                                            const std::string& tool_id) {
    return mojom::ToolUseEvent::New(tool_name, tool_id, "{}", std::nullopt,
                                    nullptr);
  }

  raw_ptr<Profile> agent_profile_ = nullptr;
  raw_ptr<content::RenderFrameHost> conversation_rfh_ = nullptr;
  raw_ptr<Browser> agent_browser_window_ = nullptr;
  raw_ptr<AIChatService> service_ = nullptr;
  raw_ptr<ConversationHandler> conversation_handler_ = nullptr;
  raw_ptr<MockEngineConsumer> mock_engine_ = nullptr;
  raw_ptr<ContentAgentToolProvider> content_agent_tool_provider_ = nullptr;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest,
                       TaskPauseResumeActions) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  NavigateToConversationUI(uuid);

  // Submit first message
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate =
        SetupMockGenerateAssistantResponse(&data_callback, &completed_callback);
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    std::move(wait_for_generate).Run();
    // Send first message response
    // Simulate tool use event
    GURL test_url = embedded_https_test_server().GetURL("/actor/link.html");
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_1", test_url)),
        std::nullopt));
    // Complete first message response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Wait for running state
  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kRunning;
  }));

  // Tool output should not be sent because we are going to pause
  EXPECT_CALL(*mock_engine_, GenerateAssistantResponse).Times(0);

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

  // Handle the tool execution response
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate =
        SetupMockGenerateAssistantResponse(&data_callback, &completed_callback);

    // Use the resume button
    ClickElement("resume-task-button");

    std::move(wait_for_generate).Run();

    // If the tool output is sent, we can verify that the tool performed
    // its action successfully.
    EXPECT_TRUE(base::test::RunUntil([this]() {
      return GetContentAgentTabHandle()
                 .Get()
                 ->GetContents()
                 ->GetLastCommittedURL()
                 .path() == "/actor/link.html";
    }));

    EXPECT_EQ(GetConversationState()->tool_use_task_state,
              mojom::TaskState::kRunning);

    // Verify the actor task tab is controlled by the agent and disabled for the
    // user.
    EXPECT_TRUE(GetActorTask()->IsUnderActorControl());

    // Send first message response
    // Simulate no more tool use requests, which should trigger task
    // completion.
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("all done")),
        std::nullopt));
    // Complete successful response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  EXPECT_TRUE(base::test::RunUntil([this]() {
    return GetConversationState()->tool_use_task_state ==
           mojom::TaskState::kNone;
  }));

  // Task state buttons should not exist anymore
  EXPECT_FALSE(VerifyElementState("task-state-actions", false));

  // EXPECT_TRUE(base::test::RunUntil([this]() {
  //   return !VerifyElementState("task-state-actions", false);
  // }));

  // When a task is complete, the actor task should be back in a ready state.
  // Instead of checking the actor task state directly, we should simply
  // check that the tab is  no longer controlled by a Task.
  EXPECT_TRUE(GetActorService()
                  ->GetTaskFromTab(*GetContentAgentTabHandle().Get())
                  .is_null())
      << "Actor task state: "
      << GetActorService()
             ->GetTask(GetActorService()->GetTaskFromTab(
                 *GetContentAgentTabHandle().Get()))
             ->GetState();
}

IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest, TaskStopAction) {
  CreateConversationWithMockEngine();
  std::string uuid = conversation_handler_->get_conversation_uuid();

  NavigateToConversationUI(uuid);

  // Submit first message
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate =
        SetupMockGenerateAssistantResponse(&data_callback, &completed_callback);
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    std::move(wait_for_generate).Run();
    // Send first message response
    // Simulate tool use event
    GURL test_url = embedded_https_test_server().GetURL("/actor/link.html");
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_1", test_url)),
        std::nullopt));
    // Complete first message response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Handle the tool execution response. We're letting the task complete one
  // round of tool execution so we can have ContentAgentToolProvider perform
  // an action in a tab and add it to the actor Task. We want to verify that
  // stopping the task does not cause any interaction issues with
  // ConversationHandler.
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate =
        SetupMockGenerateAssistantResponse(&data_callback, &completed_callback);

    EXPECT_EQ(GetConversationState()->tool_use_task_state,
              mojom::TaskState::kRunning);

    std::move(wait_for_generate).Run();

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
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Hmm, I want a different page")),
        std::nullopt));
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_2", test_url)),
        std::nullopt));
    // Complete successful response
    std::move(completed_callback)
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

  // After stopping, submit a new human message to verify that the task can be
  // re-started with new state.
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate =
        SetupMockGenerateAssistantResponse(&data_callback, &completed_callback);
    conversation_handler_->SubmitHumanConversationEntry(
        "Actually do something different", std::nullopt);

    EXPECT_TRUE(base::test::RunUntil([this]() {
      return GetConversationState()->tool_use_task_state ==
             mojom::TaskState::kNone;
    }));

    std::move(wait_for_generate).Run();
    // Send first message response
    // Simulate tool use event
    GURL test_url = embedded_https_test_server().GetURL("/actor/link.html");
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateNavigateToolUseEvent("tool_id_1", test_url)),
        std::nullopt));
    // Complete first message response
    std::move(completed_callback)
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
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate =
        SetupMockGenerateAssistantResponse(&data_callback, &completed_callback);
    std::move(wait_for_generate).Run();
  }
}

IN_PROC_BROWSER_TEST_F(AIChatConversationTaskBrowserTest, TaskUI) {
  // A task UI shows when there are 2 tool segments of a tool loop, i.e. the AI
  // responds to a tool use result with another tool use request.
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
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate = SetupMockGenerateAssistantResponse(
        &data_callback, &completed_callback, &tool_call_seq);
    conversation_handler_->SubmitHumanConversationEntry(
        "Navigate to example.com", std::nullopt);
    std::move(wait_for_generate).Run();
    // Send first message response
    // Simulate tool use event
    data_callback.Run(EngineConsumer::GenerationResultData(
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
                    CreateContentBlocksForText("1st tool result"));
              },
              std::move(callback));
        }));

    // Complete first message response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // No task UI should be shown with only one tool segment in the loop
  EXPECT_FALSE(VerifyConversationFrameElementState("assistant-task", false));
  // Handle the tool execution response with another tool use request.
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate = SetupMockGenerateAssistantResponse(
        &data_callback, &completed_callback, &tool_call_seq);
    std::move(tool_execute).Run();
    std::move(wait_for_generate).Run();

    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Hmm, I want a different thing")),
        std::nullopt));
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateToolUseEvent("mock_tool", "tool_id_2")),
        std::nullopt));

    // When the tool is being executed, we can verify the UI state
    EXPECT_CALL(*mock_tool, UseTool)
        .InSequence(tool_call_seq)
        .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
          EXPECT_TRUE(VerifyConversationFrameElementState("assistant-task"));
          EXPECT_FALSE(VerifyConversationFrameElementState(
              "tool-event-thinking", false));
          tool_execute = base::BindOnce(
              [](Tool::UseToolCallback callback) {
                std::move(callback).Run(
                    CreateContentBlocksForText("2nd tool result"));
              },
              std::move(callback));
        }));

    // Complete successful response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // Handle the second tool execution response with pausing and verify UI label.
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate = SetupMockGenerateAssistantResponse(
        &data_callback, &completed_callback, &tool_call_seq);

    // Finish executing the tool
    std::move(tool_execute).Run();

    std::move(wait_for_generate).Run();

    // Now we should be thinking
    EXPECT_TRUE(VerifyConversationFrameElementState("tool-event-thinking"));

    // Shouldn't call the tool again because we are pausing.
    EXPECT_CALL(*mock_tool, UseTool).Times(0).InSequence(tool_call_seq);

    // Pause the task
    EXPECT_TRUE(ClickElement("pause-task-button"));

    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Hmm, I want a different thing")),
        std::nullopt));
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewToolUseEvent(
            CreateToolUseEvent("mock_tool", "tool_id_3")),
        std::nullopt));
    // Complete successful response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }

  // The task should have a "paused" label
  EXPECT_TRUE(
      VerifyConversationFrameElementState("assistant-task-paused-label"));

  // When we submit a new message, the task is no longer active. It should still
  // exist but should not have its "paused" label.
  {
    EngineConsumer::GenerationDataCallback data_callback;
    EngineConsumer::GenerationCompletedCallback completed_callback;
    auto wait_for_generate = SetupMockGenerateAssistantResponse(
        &data_callback, &completed_callback, &tool_call_seq);
    conversation_handler_->SubmitHumanConversationEntry(
        "Actually do something different", std::nullopt);

    EXPECT_TRUE(base::test::RunUntil([this]() {
      return GetConversationState()->tool_use_task_state ==
             mojom::TaskState::kNone;
    }));

    std::move(wait_for_generate).Run();
    // Simple response
    data_callback.Run(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("ok")),
        std::nullopt));
    // Complete first message response
    std::move(completed_callback)
        .Run(base::ok(
            EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
  }
  EXPECT_TRUE(VerifyConversationFrameElementState("assistant-task"));
  EXPECT_FALSE(VerifyConversationFrameElementState(
      "assistant-task-paused-label", false));
}

#endif  // BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)

}  // namespace ai_chat
