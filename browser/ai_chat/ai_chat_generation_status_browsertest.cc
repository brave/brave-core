// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "base/command_line.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_timeouts.h"
#include "base/types/expected.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/public/cpp/notification.h"

namespace ai_chat {

namespace {

// What a row of the full page's conversation list shows: the data-testid of
// its indicator in conversations_list/index.tsx, each of which ends in
// "-indicator", or one of the first two values.
constexpr std::string_view kNoRow = "no row";
constexpr std::string_view kNoIndicator = "no indicator";
constexpr std::string_view kGeneratingIndicator = "generating-indicator";
constexpr std::string_view kCompletedIndicator = "completed-indicator";
constexpr std::string_view kAwaitingUserInputIndicator =
    "awaiting-user-input-indicator";

constexpr char kPermissionToolName[] = "permission_tool";

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

// Tests what the full page's conversation list and the completion notification
// show for a conversation that generates in the background, i.e. with no UI
// connected to it. Its mock engine's responses stay in progress until the test
// releases them.
class AIChatGenerationStatusBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    AIChatConversationUIBrowserTestBase::SetUpCommandLine(command_line);
    // The tests wait on pages in background tabs, whose renderers run at a
    // lower priority. On Mac CI that timed those waits out.
    command_line->AppendSwitch(switches::kDisableRendererBackgrounding);
  }

  void SetUpOnMainThread() override {
    AIChatConversationUIBrowserTestBase::SetUpOnMainThread();
    notification_tester_ = std::make_unique<NotificationDisplayServiceTester>(
        browser()->GetProfile());
    // Count each one shown, as showing one again replaces it.
    notification_tester_->SetNotificationAddedClosure(
        base::BindLambdaForTesting([this] { ++notifications_shown_; }));
  }

  void TearDownOnMainThread() override {
    notification_tester_.reset();
    AIChatConversationUIBrowserTestBase::TearDownOnMainThread();
  }

 protected:
  // The completion notifications currently shown.
  std::vector<message_center::Notification> GetNotifications() {
    return notification_tester_->GetDisplayedNotificationsForType(
        NotificationHandler::Type::TRANSIENT);
  }

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

  // Adds a tool named kPermissionToolName to the background conversation,
  // which asks for the user's permission until it is granted.
  MockTool* AddPermissionTool() {
    auto* tool_provider = conversation_handler_->AddToolProviderForTesting(
        std::make_unique<testing::NiceMock<MockToolProvider>>());
    auto* tool = tool_provider->AddToolForTesting(
        std::make_unique<testing::NiceMock<MockTool>>(kPermissionToolName));
    auto granted = std::make_shared<bool>(false);
    ON_CALL(*tool, RequiresUserInteractionBeforeHandling)
        .WillByDefault(
            [granted](const mojom::ToolUseEvent&)
                -> std::variant<bool, mojom::PermissionChallengePtr> {
              if (*granted) {
                return false;
              }
              return mojom::PermissionChallenge::New();
            });
    ON_CALL(*tool, UserPermissionGranted).WillByDefault([granted](auto&&...) {
      *granted = true;
    });
    return tool;
  }

  // Releases a held response with a request to use the permission tool.
  void RequestPermissionToolUse(MockGenerateCallbacks response) {
    ReleaseResponse(std::move(response),
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        CreateToolUseEvent(kPermissionToolName, "tool_use")));
  }

  // Content ignores hidden updates for a WebContents until it has first been
  // shown for real, which can fail to happen when the test's window isn't on
  // screen, as on some bots; a tab switch there hides nothing. Call this while
  // |contents| shows. The test then sets the visibility a switch would give it
  // with UpdateWebContentsVisibility(). Only hide contents that aren't in the
  // window, such as a background tab or an inactive side panel entry, so that
  // no update from the window can show them again.
  static void MarkShown(content::WebContents* contents) {
    contents->UpdateWebContentsVisibility(content::Visibility::VISIBLE);
  }

  // Closes the page NavigateToConversationUI() last opened and waits until the
  // background conversation has no client connected.
  void CloseConversationUI() {
    TabStripModel* tab_strip = browser()->GetTabStripModel();
    const int index = tab_strip->GetIndexOfWebContents(
        content::WebContents::FromRenderFrameHost(conversation_rfh_));
    conversation_rfh_ = nullptr;
    tab_strip->CloseWebContentsAt(index, TabCloseTypes::CLOSE_NONE);
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return !conversation_handler_->IsAnyClientConnected(); }));
  }

  // Streams |event| as part of a held response, without releasing it.
  static void SendEvent(const MockGenerateCallbacks& response,
                        mojom::ConversationEntryEventPtr event) {
    response.data_callback.Run(
        EngineConsumer::GenerationResultData(std::move(event), std::nullopt));
  }

  // Releases a held response with |event| as its last content. After a tool
  // use event the conversation runs the tool and then asks the engine for the
  // next response, so hold that one first with
  // SetupMockGenerateAssistantResponse().
  static void ReleaseResponse(MockGenerateCallbacks response,
                              mojom::ConversationEntryEventPtr event) {
    SendEvent(response, std::move(event));
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

  std::unique_ptr<NotificationDisplayServiceTester> notification_tester_;
  // How many times a notification has been shown.
  int notifications_shown_ = 0;
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

// A response that finishes in the background shows one notification, with the
// conversation's title as its body.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       BackgroundCompletionShowsNotification) {
  content::RenderFrameHost* page = OpenFullPageOnNewChat();

  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kGeneratingIndicator));
  EXPECT_EQ(notifications_shown_, 0);

  SendEvent(response, mojom::ConversationEntryEvent::NewConversationTitleEvent(
                          mojom::ConversationTitleEvent::New("Rivers")));
  CompleteResponse(std::move(response));
  // The service shows the notification as it sends the state that adds the
  // dot, so it is there once the dot is.
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kCompletedIndicator));

  EXPECT_EQ(notifications_shown_, 1);
  const auto notifications = GetNotifications();
  ASSERT_EQ(notifications.size(), 1u);
  EXPECT_EQ(notifications[0].id(), base::StrCat({"ai_chat.", uuid}));
  EXPECT_EQ(notifications[0].title(),
            l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_TITLE));
  EXPECT_EQ(notifications[0].message(), u"Rivers");
}

// Opening the conversation from the page closes its notification, whose body
// is a generic message when the conversation has no title.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       OpeningConversationClosesNotification) {
  content::RenderFrameHost* page = OpenFullPageOnNewChat();

  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();
  CompleteResponse(std::move(response));
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kCompletedIndicator));

  const auto notifications = GetNotifications();
  ASSERT_EQ(notifications.size(), 1u);
  EXPECT_EQ(notifications[0].message(),
            l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_BODY_FALLBACK));

  ASSERT_TRUE(ClickElement(base::StrCat({"conversation-item-", uuid}), page));
  EXPECT_TRUE(base::test::RunUntil([&] { return GetNotifications().empty(); }));
  EXPECT_EQ(notifications_shown_, 1);
}

// A response that finishes while its conversation is open shows no
// notification.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       NoNotificationWhenViewedAsItFinishes) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();

  content::RenderFrameHost* other_page = OpenFullPageOnNewChat();
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kGeneratingIndicator));

  NavigateToConversationUI(uuid);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return conversation_handler_->IsAnyClientConnected(); }));

  CompleteResponse(std::move(response));
  // The other page's ring goes once the service has decided the response
  // finished while viewed, which is when it would show a notification.
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
  EXPECT_EQ(notifications_shown_, 0);
  EXPECT_TRUE(GetNotifications().empty());
}

// A background conversation that stops on a tool's permission challenge shows
// the warning rather than the dot, and a notification asking for approval.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       PermissionRequestShowsAwaitingIndicator) {
  content::RenderFrameHost* page = OpenFullPageOnNewChat();

  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();
  MockTool* tool = AddPermissionTool();
  EXPECT_CALL(*tool, UseTool).Times(0);
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kGeneratingIndicator));

  RequestPermissionToolUse(std::move(response));
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kAwaitingUserInputIndicator));

  const auto notifications = GetNotifications();
  ASSERT_EQ(notifications.size(), 1u);
  EXPECT_EQ(
      notifications[0].title(),
      l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_TITLE_NEEDS_APPROVAL));
  EXPECT_EQ(notifications[0].message(),
            l10n_util::GetStringUTF16(
                IDS_AI_CHAT_NOTIFICATION_BODY_NEEDS_APPROVAL_FALLBACK));
}

// Approving the challenge from the conversation brings the ring back while the
// tool runs and the response continues, and the dot once that finishes with
// the conversation no longer open.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       ApprovingPermissionShowsGeneratingThenCompleted) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();
  MockTool* tool = AddPermissionTool();

  // This page keeps its new chat open throughout.
  content::RenderFrameHost* other_page = OpenFullPageOnNewChat();
  RequestPermissionToolUse(std::move(response));
  ASSERT_TRUE(
      WaitForRowIndicator(other_page, uuid, kAwaitingUserInputIndicator));

  // Opening the conversation clears the warning.
  NavigateToConversationUI(uuid);
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));

  Tool::UseToolCallback use_tool;
  EXPECT_CALL(*tool, UseTool)
      .WillOnce(testing::WithArg<1>([&](Tool::UseToolCallback callback) {
        use_tool = std::move(callback);
      }));
  ASSERT_TRUE(
      VerifyConversationFrameElementState("permission-allow-once-button"));
  ASSERT_TRUE(ClickElement("permission-allow-once-button",
                           GetConversationEntriesFrame()));
  ASSERT_TRUE(base::test::RunUntil([&] { return !use_tool.is_null(); }));
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kGeneratingIndicator));

  // Leave while the tool runs, then let it and the response finish.
  CloseConversationUI();
  auto future = SetupMockGenerateAssistantResponse();
  std::move(use_tool).Run(CreateContentBlocksForText("Tool result"), {});
  MockGenerateCallbacks next_response = future->Take();
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kGeneratingIndicator));

  CompleteResponse(std::move(next_response));
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kCompletedIndicator));
  const auto notifications = GetNotifications();
  ASSERT_EQ(notifications.size(), 1u);
  EXPECT_EQ(notifications[0].title(),
            l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_TITLE));
}

// Leaving a conversation without answering its challenge brings the warning
// back, but not once its task has been stopped.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       StoppingTaskClearsAwaitingIndicator) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  const std::string uuid = conversation_handler_->get_conversation_uuid();
  MockTool* tool = AddPermissionTool();
  EXPECT_CALL(*tool, UseTool).Times(0);

  // This page keeps its new chat open throughout.
  content::RenderFrameHost* other_page = OpenFullPageOnNewChat();
  RequestPermissionToolUse(std::move(response));
  ASSERT_TRUE(
      WaitForRowIndicator(other_page, uuid, kAwaitingUserInputIndicator));

  NavigateToConversationUI(uuid);
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
  CloseConversationUI();
  ASSERT_TRUE(
      WaitForRowIndicator(other_page, uuid, kAwaitingUserInputIndicator));

  NavigateToConversationUI(uuid);
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
  // The page only shows a stop control for content agent tasks, so call what
  // it calls.
  conversation_handler_->StopTask();
  CloseConversationUI();

  // Finish another background response. The service sends its dot after
  // anything it sent when the stopped conversation was left, so once the dot
  // shows, the stopped conversation's row is up to date.
  MockGenerateCallbacks other_response = StartBackgroundGeneration();
  const std::string other_uuid = conversation_handler_->get_conversation_uuid();
  CompleteResponse(std::move(other_response));
  ASSERT_TRUE(WaitForRowIndicator(other_page, other_uuid, kCompletedIndicator));
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
}

// A full page left open on the conversation in a background tab stays
// connected but doesn't count as viewing it: the conversation finishes with the
// dot and one notification, and activating that tab again clears both.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       HiddenFullPageDoesNotViewConversation) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  ConversationHandler* handler = conversation_handler_;
  const std::string uuid = handler->get_conversation_uuid();

  NavigateToConversationUI(uuid);
  content::WebContents* conversation_contents =
      content::WebContents::FromRenderFrameHost(conversation_rfh_);
  MarkShown(conversation_contents);
  const int conversation_tab = browser()->GetTabStripModel()->active_index();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return handler->IsAnyClientVisible(); }));

  // Opening another tab hides the conversation's tab.
  content::RenderFrameHost* other_page = OpenFullPageOnNewChat();
  conversation_contents->UpdateWebContentsVisibility(
      content::Visibility::HIDDEN);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !handler->IsAnyClientVisible(); }));
  EXPECT_TRUE(handler->IsAnyClientConnected());
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kGeneratingIndicator));

  CompleteResponse(std::move(response));
  // The service shows the notification as it sends the state that adds the
  // dot, so it is there once the dot is.
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kCompletedIndicator));
  EXPECT_EQ(notifications_shown_, 1);
  const auto notifications = GetNotifications();
  ASSERT_EQ(notifications.size(), 1u);
  EXPECT_EQ(notifications[0].id(), base::StrCat({"ai_chat.", uuid}));

  browser()->GetTabStripModel()->ActivateTabAt(conversation_tab);
  conversation_contents->UpdateWebContentsVisibility(
      content::Visibility::VISIBLE);
  // Likewise it closes the notification as it sends the state that clears it.
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
  EXPECT_TRUE(GetNotifications().empty());
  EXPECT_EQ(notifications_shown_, 1);
}

// The same for the side panel. Showing another entry in it keeps the
// conversation's contents loaded but out of the window, so they are hidden
// while still connected; showing the conversation again brings them back.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       HiddenSidePanelDoesNotViewConversation) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  ConversationHandler* handler = conversation_handler_;
  const std::string uuid = handler->get_conversation_uuid();
  content::RenderFrameHost* page = OpenFullPageOnNewChat();

  OpenConversationInSidePanel(browser()->GetProfile(), uuid);
  content::WebContents* side_panel_contents =
      GetSidePanelWebContents(browser());
  ASSERT_TRUE(side_panel_contents);
  base::WeakPtr<content::WebContents> side_panel =
      side_panel_contents->GetWeakPtr();
  MarkShown(side_panel.get());
  ASSERT_TRUE(
      base::test::RunUntil([&] { return handler->IsAnyClientVisible(); }));

  SidePanelUI* side_panel_ui = SidePanelUI::From(browser());
  side_panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return GetSidePanelWebContents(browser()) != side_panel.get(); }));
  // The side panel keeps the conversation's view while another entry shows.
  ASSERT_TRUE(side_panel);
  side_panel->UpdateWebContentsVisibility(content::Visibility::HIDDEN);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return !handler->IsAnyClientVisible(); }));
  EXPECT_TRUE(handler->IsAnyClientConnected());

  CompleteResponse(std::move(response));
  ASSERT_TRUE(WaitForRowIndicator(page, uuid, kCompletedIndicator));
  EXPECT_EQ(notifications_shown_, 1);
  EXPECT_EQ(GetNotifications().size(), 1u);

  side_panel_ui->Show(SidePanelEntryId::kChatUI);
  ASSERT_TRUE(base::test::RunUntil([&] {
    return side_panel && GetSidePanelWebContents(browser()) == side_panel.get();
  }));
  side_panel->UpdateWebContentsVisibility(content::Visibility::VISIBLE);
  EXPECT_TRUE(WaitForRowIndicator(page, uuid, kNoIndicator));
  EXPECT_TRUE(GetNotifications().empty());
  EXPECT_EQ(notifications_shown_, 1);
}

// Switching away from a conversation's tab without answering its challenge
// brings the warning back while the page stays connected, and switching back
// clears it again. A challenge that arrives while viewed shows no notification.
IN_PROC_BROWSER_TEST_F(AIChatGenerationStatusBrowserTest,
                       HidingUnansweredChallengeShowsAwaitingIndicator) {
  MockGenerateCallbacks response = StartBackgroundGeneration();
  ConversationHandler* handler = conversation_handler_;
  const std::string uuid = handler->get_conversation_uuid();
  MockTool* tool = AddPermissionTool();
  EXPECT_CALL(*tool, UseTool).Times(0);

  content::RenderFrameHost* other_page = OpenFullPageOnNewChat();
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kGeneratingIndicator));
  TabStripModel* tab_strip = browser()->GetTabStripModel();
  const int other_tab = tab_strip->GetIndexOfWebContents(
      content::WebContents::FromRenderFrameHost(other_page));

  NavigateToConversationUI(uuid);
  content::WebContents* conversation_contents =
      content::WebContents::FromRenderFrameHost(conversation_rfh_);
  MarkShown(conversation_contents);
  const int conversation_tab = tab_strip->active_index();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return handler->IsAnyClientVisible(); }));

  // The ring goes once the service has decided the response stopped on the
  // challenge while viewed.
  RequestPermissionToolUse(std::move(response));
  ASSERT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
  ASSERT_TRUE(
      VerifyConversationFrameElementState("permission-allow-once-button"));

  tab_strip->ActivateTabAt(other_tab);
  conversation_contents->UpdateWebContentsVisibility(
      content::Visibility::HIDDEN);
  EXPECT_TRUE(
      WaitForRowIndicator(other_page, uuid, kAwaitingUserInputIndicator));
  EXPECT_TRUE(handler->IsAnyClientConnected());

  tab_strip->ActivateTabAt(conversation_tab);
  conversation_contents->UpdateWebContentsVisibility(
      content::Visibility::VISIBLE);
  EXPECT_TRUE(WaitForRowIndicator(other_page, uuid, kNoIndicator));
  EXPECT_EQ(notifications_shown_, 0);
}

}  // namespace ai_chat
