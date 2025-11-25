/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/mojom/menu_source_type.mojom.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PDF)
#include "chrome/browser/pdf/pdf_extension_test_util.h"
#endif  // BUILDFLAG(ENABLE_PDF)

using ::testing::_;

namespace ai_chat {

namespace {

void ExecuteRewriteCommand(RenderViewContextMenu* context_menu) {
  // Calls EngineConsumer::GenerateRewriteSuggestion
  context_menu->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SHORTEN, 0);
  context_menu->Cancel();
}

class MockConversationHandlerClient : public mojom::ConversationUI {
 public:
  explicit MockConversationHandlerClient(ConversationHandler* driver) {
    driver->Bind(conversation_handler_.BindNewPipeAndPassReceiver(),
                 conversation_ui_receiver_.BindNewPipeAndPassRemote());
  }

  ~MockConversationHandlerClient() override = default;

  void Disconnect() {
    conversation_handler_.reset();
    conversation_ui_receiver_.reset();
  }

  MOCK_METHOD(void,
              OnConversationHistoryUpdate,
              (const mojom::ConversationTurnPtr),
              (override));

  MOCK_METHOD(void, OnAPIRequestInProgress, (bool), (override));

  MOCK_METHOD(void, OnAPIResponseError, (mojom::APIError), (override));

  MOCK_METHOD(void,
              OnTaskStateChanged,
              (mojom::TaskState task_state),
              (override));

  MOCK_METHOD(void,
              OnModelDataChanged,
              (const std::string& conversation_model_key,
               const std::string& default_model_key,
               std::vector<mojom::ModelPtr> all_models),
              (override));

  MOCK_METHOD(void,
              OnSuggestedQuestionsChanged,
              (const std::vector<std::string>&,
               mojom::SuggestionGenerationStatus),
              (override));

  MOCK_METHOD(void,
              OnAssociatedContentInfoChanged,
              (std::vector<mojom::AssociatedContentPtr>),
              (override));

  MOCK_METHOD(void, OnConversationDeleted, (), (override));

 private:
  mojo::Receiver<mojom::ConversationUI> conversation_ui_receiver_{this};
  mojo::Remote<mojom::ConversationHandler> conversation_handler_;
};

}  // namespace

class AIChatRenderViewContextMenuBrowserTest : public InProcessBrowserTest {
 public:
  AIChatRenderViewContextMenuBrowserTest()
      : ai_engine_(std::make_unique<MockEngineConsumer>()),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~AIChatRenderViewContextMenuBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir.AppendASCII("ai_chat"));
    https_server_.ServeFilesFromDirectory(test_data_dir.AppendASCII("leo"));
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

  void TestRewriteInPlace(
      content::WebContents* web_contents,
      const std::string& element_id,
      const std::string& expected_selected_text,
      const std::vector<std::string>& received_data,
      base::expected<std::string, mojom::APIError> completed_result,
      const std::string& expected_updated_text) {
    base::RunLoop run_loop;
    MockEngineConsumer* ai_engine;

    // Select text in the element and create context menu to execute a rewrite
    // command.
    std::string selected_text =
        content::EvalJs(web_contents,
                        content::JsReplace("select_all($1)", element_id))
            .ExtractString();
    EXPECT_EQ(expected_selected_text, selected_text);

    int x = content::EvalJs(web_contents,
                            content::JsReplace("getRectX($1)", element_id))
                .ExtractInt();
    int y = content::EvalJs(web_contents,
                            content::JsReplace("getRectY($1)", element_id))
                .ExtractInt();

    RenderViewContextMenu::RegisterMenuShownCallbackForTesting(
        base::BindLambdaForTesting([&](RenderViewContextMenu* context_menu) {
          auto* brave_context_menu =
              static_cast<BraveRenderViewContextMenu*>(context_menu);
          brave_context_menu->SetAIEngineForTesting(
              std::make_unique<MockEngineConsumer>());
          ai_engine = static_cast<MockEngineConsumer*>(
              brave_context_menu->GetAIEngineForTesting());
          // Verify that rewrite is requested
          EXPECT_CALL(*ai_engine, GenerateRewriteSuggestion(_, _, _, _, _))
              .WillOnce(
                  [&](const std::string& text, mojom::ActionType action_type,
                      const std::string& selected_language,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback) {
                    ASSERT_TRUE(callback);
                    ASSERT_TRUE(data_callback);
                    for (const auto& data : received_data) {
                      auto event =
                          mojom::ConversationEntryEvent::NewCompletionEvent(
                              mojom::CompletionEvent::New(data));
                      data_callback.Run(EngineConsumer::GenerationResultData(
                          std::move(event), std::nullopt /* model_key */));
                    }

                    if (completed_result.has_value()) {
                      auto event =
                          mojom::ConversationEntryEvent::NewCompletionEvent(
                              mojom::CompletionEvent::New(
                                  completed_result.value()));
                      std::move(callback).Run(
                          base::ok(EngineConsumer::GenerationResultData(
                              std::move(event), std::nullopt /* model_key */)));
                    } else {
                      std::move(callback).Run(
                          base::unexpected(completed_result.error()));
                    }

                    run_loop.Quit();
                  });
          base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
              FROM_HERE, base::BindOnce(&ExecuteRewriteCommand, context_menu));
        }));

    web_contents->GetPrimaryMainFrame()
        ->GetRenderViewHost()
        ->GetWidget()
        ->ShowContextMenuAtPoint(gfx::Point(x, y),
                                 ui::mojom::MenuSourceType::kMouse);
    run_loop.Run();
    EXPECT_NE(ai_engine, nullptr);
    testing::Mock::VerifyAndClearExpectations(ai_engine);

    // Verify that the text is rewritten as expected.
    std::string updated_text =
        content::EvalJs(web_contents,
                        content::JsReplace("get_text($1)", element_id))
            .ExtractString();
    EXPECT_EQ(expected_updated_text, updated_text);
  }

  sidebar::SidebarController* GetSidebarController() {
    auto* controller = browser()->GetFeatures().sidebar_controller();
    EXPECT_TRUE(controller);
    return controller;
  }

  bool IsAIChatSidebarActive() {
    auto* sidebar_controller = GetSidebarController();
    auto index = sidebar_controller->model()->GetIndexOf(
        sidebar::SidebarItem::BuiltInItemType::kChatUI);
    return sidebar_controller->IsActiveIndex(index);
  }

  ConversationHandler* GetConversationHandler() {
    AIChatTabHelper* helper = AIChatTabHelper::FromWebContents(web_contents());
    if (!helper) {
      return nullptr;
    }

    return AIChatServiceFactory::GetInstance()
        ->GetForBrowserContext(browser()->profile())
        ->GetOrCreateConversationHandlerForContent(
            helper->web_contents_content().content_id(),
            helper->web_contents_content().GetWeakPtr());
  }

  std::unique_ptr<testing::NiceMock<MockConversationHandlerClient>>
  SetupMockConversationHandler(const base::Location& location) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    ConversationHandler* conversation_handler = GetConversationHandler();
    if (!conversation_handler) {
      ADD_FAILURE() << "Could not get ConversationHandler";
      return nullptr;
    }

    return std::make_unique<testing::NiceMock<MockConversationHandlerClient>>(
        conversation_handler);
  }

  // This is to wait for the conversation history update event.
  // Note that this event would only happen in non rewrite-in-place path.
  void ListenForConversationHistoryUpdate(
      testing::NiceMock<MockConversationHandlerClient>& client,
      base::RunLoop& run_loop,
      std::string& submitted_text,
      const base::Location& location) {
    EXPECT_CALL(client, OnConversationHistoryUpdate(_))
        .WillOnce([&, location](const mojom::ConversationTurnPtr turn) {
          SCOPED_TRACE(testing::Message() << location.ToString());
          ConversationHandler* conversation_handler = GetConversationHandler();
          ASSERT_TRUE(conversation_handler);
          auto& history = conversation_handler->GetConversationHistory();
          ASSERT_EQ(history.size(), 1u);
          auto& entry = history[0];
          ASSERT_TRUE(entry);
          ASSERT_TRUE(entry->selected_text);
          submitted_text = *entry->selected_text;
          run_loop.Quit();
        });
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenu(
      content::RenderFrameHost* target_frame,
      bool is_editable,
      const std::u16string& selection_text) {
    content::ContextMenuParams params;
    params.is_editable = is_editable;
    params.selection_text = selection_text;
    auto menu =
        std::make_unique<TestRenderViewContextMenu>(*target_frame, params);
    menu->Init();
    return menu;
  }

 private:
  std::unique_ptr<MockEngineConsumer> ai_engine_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(AIChatRenderViewContextMenuBrowserTest, RewriteInPlace) {
  // Mimic user opt-in by setting pref.
  SetUserOptedIn(GetPrefs(), true);

  // Load rewrite.html
  GURL url = https_server()->GetURL("/rewrite.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* contents = web_contents();

  // This is to keep the ConversationHandler alive until the test is done.
  auto client = SetupMockConversationHandler(FROM_HERE);
  ASSERT_TRUE(client);

  // Test rewriting textarea value.
  TestRewriteInPlace(contents, "textarea", "I'm textarea.",
                     {"This", " is", " the w", "ay."}, "", "This is the way.");

  // Do the same again to make sure it still works at the second time.
  TestRewriteInPlace(contents, "textarea", "This is the way.", {"OK", "2"}, "",
                     "OK2");

  // Error case handling tests and verify that the text is not rewritten.
  // 1) Get error in completed callback immediately.
  EXPECT_FALSE(IsAIChatSidebarActive());
  TestRewriteInPlace(contents, "textarea", "OK2", {},
                     base::unexpected(mojom::APIError::ConnectionIssue), "OK2");
  EXPECT_TRUE(IsAIChatSidebarActive());
  GetSidebarController()->DeactivateCurrentPanel();

  EXPECT_FALSE(IsAIChatSidebarActive());
  // 2) Get partial streaming responses then error in completed callback.
  TestRewriteInPlace(contents, "textarea", "OK2", {"N", "O"},
                     base::unexpected(mojom::APIError::ConnectionIssue), "OK2");
  EXPECT_TRUE(IsAIChatSidebarActive());
}

IN_PROC_BROWSER_TEST_F(AIChatRenderViewContextMenuBrowserTest,
                       RewriteInPlace_InputText) {
  // Mimic user opt-in by setting pref.
  SetUserOptedIn(GetPrefs(), true);

  // Load rewrite.html
  GURL url = https_server()->GetURL("/rewrite.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* contents = web_contents();

  // This is to keep the ConversationHandler alive until the test is done.
  auto client = SetupMockConversationHandler(FROM_HERE);
  ASSERT_TRUE(client);

  // Test rewriting text input.
  TestRewriteInPlace(contents, "input_text", "I'm input.", {"OK", "3"}, "",
                     "OK3");
}

IN_PROC_BROWSER_TEST_F(AIChatRenderViewContextMenuBrowserTest,
                       RewriteInPlace_ContentEditable) {
  // Mimic user opt-in by setting pref.
  SetUserOptedIn(GetPrefs(), true);

  // Load rewrite.html
  GURL url = https_server()->GetURL("/rewrite.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* contents = web_contents();

  // This is to keep the ConversationHandler alive until the test is done.
  auto client = SetupMockConversationHandler(FROM_HERE);
  ASSERT_TRUE(client);

  // Test rewriting contenteditable.
  TestRewriteInPlace(contents, "contenteditable", "I'm contenteditable.",
                     {"OK4"}, "", "OK4");
}

IN_PROC_BROWSER_TEST_F(AIChatRenderViewContextMenuBrowserTest,
                       SubmitSelectedText) {
  // Mimic user opt-in by setting pref.
  SetUserOptedIn(GetPrefs(), true);

  GURL url = https_server()->GetURL("/text.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* contents = web_contents();

  // Setup a mock completion client to handle the request.
  auto client = SetupMockConversationHandler(FROM_HERE);
  ASSERT_TRUE(client);

  base::RunLoop run_loop;
  std::string submitted_text;
  ListenForConversationHistoryUpdate(*client, run_loop, submitted_text,
                                     FROM_HERE);

  // Create a context menu with selected text.
  content::RenderFrameHost* target_frame = contents->GetPrimaryMainFrame();
  auto menu = CreateContextMenu(target_frame, /*is_editable=*/false,
                                u"This is the way");

  ASSERT_TRUE(menu->IsCommandIdEnabled(IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT));

  // Execute the command.
  menu->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT, 0);
  run_loop.Run();

  EXPECT_EQ(submitted_text, "This is the way");
  EXPECT_TRUE(IsAIChatSidebarActive());
}

#if BUILDFLAG(ENABLE_PDF)
IN_PROC_BROWSER_TEST_F(AIChatRenderViewContextMenuBrowserTest,
                       SubmitSelectedTextInPDF) {
  // Mimic user opt-in by setting pref.
  SetUserOptedIn(GetPrefs(), true);

  // Load a dummy PDF page.
  GURL url = https_server()->GetURL("/dummy.pdf");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_TRUE(pdf_extension_test_util::EnsurePDFHasLoaded(web_contents()));

  // Setup a mock client to listen to the conversation history update.
  auto client = SetupMockConversationHandler(FROM_HERE);
  ASSERT_TRUE(client);

  base::RunLoop run_loop;
  std::string submitted_text;
  ListenForConversationHistoryUpdate(*client, run_loop, submitted_text,
                                     FROM_HERE);

  // Create a context menu on PDF frame with selected text.
  content::RenderFrameHost* target_frame =
      pdf_extension_test_util::GetOnlyPdfPluginFrame(web_contents());
  auto menu = CreateContextMenu(target_frame, /*is_editable=*/false,
                                u"This is the way");

  ASSERT_TRUE(menu->IsCommandIdEnabled(IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT));

  // Execute the command.
  menu->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT, 0);
  run_loop.Run();

  EXPECT_EQ(submitted_text, "This is the way");
  EXPECT_TRUE(IsAIChatSidebarActive());
}

// Rewrite commands in PDF would always go through the same path as
// SubmitSelectedTextInPDF currently because the rewrite in place
// implementation does not support PDF. This test is to verify that
// it would work by going through the same path as SubmitSelectedTextInPDF.
IN_PROC_BROWSER_TEST_F(AIChatRenderViewContextMenuBrowserTest,
                       RewriteInPlaceDisabledInPDF) {
  // Mimic user opt-in by setting pref.
  SetUserOptedIn(GetPrefs(), true);

  // Load a dummy PDF page.
  GURL url = https_server()->GetURL("/dummy.pdf");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_TRUE(pdf_extension_test_util::EnsurePDFHasLoaded(web_contents()));

  auto client = SetupMockConversationHandler(FROM_HERE);
  ASSERT_TRUE(client);

  base::RunLoop run_loop;
  std::string submitted_text;
  ListenForConversationHistoryUpdate(*client, run_loop, submitted_text,
                                     FROM_HERE);

  // Create a context menu on PDF frame with selected text.
  content::RenderFrameHost* target_frame =
      pdf_extension_test_util::GetOnlyPdfPluginFrame(web_contents());
  auto menu =
      CreateContextMenu(target_frame, /*is_editable=*/true, u"This is the way");

  ASSERT_TRUE(menu->IsCommandIdEnabled(IDC_AI_CHAT_CONTEXT_SHORTEN));

  // Execute the command.
  menu->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SHORTEN, 0);
  run_loop.Run();

  EXPECT_EQ(submitted_text, "This is the way");
  EXPECT_TRUE(IsAIChatSidebarActive());
}
#endif  // BUILDFLAG(ENABLE_PDF)

}  // namespace ai_chat
