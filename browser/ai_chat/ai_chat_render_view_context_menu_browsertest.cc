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
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_remote_completion_client.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
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
#include "url/gurl.h"

using ::testing::_;

namespace ai_chat {

namespace {

void ExecuteRewriteCommand(RenderViewContextMenu* context_menu) {
  // Calls EngineConsumer::GenerateRewriteSuggestion
  context_menu->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SHORTEN, 0);
  context_menu->Cancel();
}

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
        base::PathService::CheckedGet(brave::DIR_TEST_DATA)
            .AppendASCII("ai_chat");
    https_server_.ServeFilesFromDirectory(test_data_dir);
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
                  [&](std::string text, const std::string& question,
                      const std::string& selected_language,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback) {
                    ASSERT_TRUE(callback);
                    ASSERT_TRUE(data_callback);
                    for (const auto& data : received_data) {
                      auto event =
                          mojom::ConversationEntryEvent::NewCompletionEvent(
                              mojom::CompletionEvent::New(data));
                      data_callback.Run(std::move(event));
                    }
                    std::move(callback).Run(completed_result);
                    run_loop.Quit();
                  });
          base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
              FROM_HERE, base::BindOnce(&ExecuteRewriteCommand, context_menu));
        }));

    web_contents->GetPrimaryMainFrame()
        ->GetRenderViewHost()
        ->GetWidget()
        ->ShowContextMenuAtPoint(gfx::Point(x, y), ui::MENU_SOURCE_MOUSE);
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
    auto* controller =
        static_cast<BraveBrowser*>(browser())->sidebar_controller();
    EXPECT_TRUE(controller);
    return controller;
  }

  bool IsAIChatSidebarActive() {
    auto* sidebar_controller = GetSidebarController();
    auto index = sidebar_controller->model()->GetIndexOf(
        sidebar::SidebarItem::BuiltInItemType::kChatUI);
    return sidebar_controller->IsActiveIndex(index);
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

  // Setup a mock completion client to handle the request.
  ai_chat::AIChatTabHelper* helper =
      ai_chat::AIChatTabHelper::FromWebContents(contents);
  ASSERT_TRUE(helper);

  ConversationHandler* conversation_handler =
      ai_chat::AIChatServiceFactory::GetInstance()
          ->GetForBrowserContext(browser()->profile())
          ->GetOrCreateConversationHandlerForContent(helper->GetContentId(),
                                                     helper->GetWeakPtr());
  ASSERT_TRUE(conversation_handler);

  // Test rewriting textarea value and verify that the response tag is ignored
  // by BraveRenderViewContextMenu
  TestRewriteInPlace(contents, "textarea", "I'm textarea.",
                     {"O", "OK", "<", "</", "</r", "</re", "</response"}, "",
                     "OK");

  // Do the same again to make sure it still works at the second time.
  TestRewriteInPlace(contents, "textarea", "OK", {"O", "OK", "OK2"}, "", "OK2");

  // Select text in text input and create context menu to execute a rewrite cmd.
  // Verify that the text is rewritten.
  TestRewriteInPlace(contents, "input_text", "I'm input.", {"O", "OK", "OK3"},
                     "", "OK3");
  TestRewriteInPlace(contents, "contenteditable", "I'm contenteditable.",
                     {"O", "OK", "OK4"}, "", "OK4");

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

}  // namespace ai_chat
