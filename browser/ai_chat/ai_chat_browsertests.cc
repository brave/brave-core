/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/content/browser/ai_page_content_fetcher.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatBrowserTest : public InProcessBrowserTest {
 public:
  AIChatBrowserTest() = default;

  void SetUpOnMainThread() override {
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir.AppendASCII("ai_chat"));

    // Add handler for YouTube player endpoint
    https_server_.RegisterRequestHandler(base::BindLambdaForTesting(
        [test_data_dir](const net::test_server::HttpRequest& request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          if (base::StartsWith(request.relative_url, "/youtubei/v1/player")) {
            auto response =
                std::make_unique<net::test_server::BasicHttpResponse>();

            // Extract videoId from the JSON request body
            std::string video_id;
            if (request.method == net::test_server::METHOD_POST &&
                !request.content.empty()) {
              // Parse the JSON body to extract videoId
              auto json_result = base::JSONReader::Read(
                  request.content, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
              if (json_result.has_value() && json_result->is_dict()) {
                const auto& body_dict = json_result->GetDict();
                if (const std::string* video_id_ptr =
                        body_dict.FindString("videoId")) {
                  video_id = *video_id_ptr;
                }
              }
            }

            base::FilePath player_dir = test_data_dir.AppendASCII("ai_chat")
                                            .AppendASCII("youtubei")
                                            .AppendASCII("v1")
                                            .AppendASCII("player_dir");

            // Determine which file to serve based on videoId
            base::FilePath file_path;
            if (!video_id.empty()) {
              file_path =
                  player_dir.AppendASCII(base::StrCat({video_id, ".json"}));
            } else {
              file_path = player_dir.AppendASCII("default.json");
            }

            // Read and serve the file
            std::string file_contents;
            CHECK(base::ReadFileToString(file_path, &file_contents))
                << "Failed to read file: " << file_path;
            response->set_code(net::HTTP_OK);
            response->set_content_type("application/json");
            response->set_content(file_contents);
            return response;
          }
          return nullptr;  // Let the server handle other requests
        }));

    https_server_.StartAcceptingConnections();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void TearDownOnMainThread() override {
    // AIPageContentFetcher holds a raw_ptr to the active WebContents, so reset
    // it before the browser tears the WebContents down to avoid a dangling
    // raw_ptr.
    ai_page_content_fetcher_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* ActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  std::string FetchPageContent() {
    page_content_fetcher_ = std::make_unique<PageContentFetcher>(
        browser()->tab_strip_model()->GetActiveWebContents());
    base::test::TestFuture<std::string, bool, std::string> future;
    page_content_fetcher_->FetchPageContent("", future.GetCallback());
    return future.Get<0>();
  }

  // Extracts the page content via AIPageContentFetcher, which goes through
  // Chromium's optimization_guide::GetAIPageContent and our
  // ConvertAnnotatedPageContentToBlocks conversion. Unlike FetchPageContent()
  // above (the renderer IPC path), this exercises the real GetAIPageContent
  // data, so the test fails if Chromium's extraction output changes.
  std::string FetchAIPageContent() {
    ai_page_content_fetcher_ = std::make_unique<AIPageContentFetcher>(
        browser()->tab_strip_model()->GetActiveWebContents());
    base::test::TestFuture<std::string, bool, std::string> future;
    ai_page_content_fetcher_->FetchPageContent("", future.GetCallback());
    return future.Get<0>();
  }

 private:
  std::unique_ptr<PageContentFetcher> page_content_fetcher_;
  std::unique_ptr<AIPageContentFetcher> ai_page_content_fetcher_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(AIChatBrowserTest, YoutubeNavigations) {
  const GURL url("https://www.youtube.com/youtube.html?v=video_id_001");

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const std::string initial_content = FetchPageContent();
  EXPECT_EQ("Initial content", initial_content);

  // Also test regex fallback
  const GURL url2(
      "https://www.youtube.com/youtube-fallback.html?v=video_id_002");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const std::string navigated_content = FetchPageContent();
  EXPECT_EQ("Navigated content", navigated_content);
}

// Extracts a regular HTML page via the real GetAIPageContent path and verifies
// the readable content is present while the agentic targeting metadata is
// stripped (AI Chat uses PageContentDetail::kContentOnly). This is the
// content-only counterpart to content_agent_tool_provider_browsertest.cc, which
// loads the same page through the agentic path and asserts the dom_ids ARE
// present. Together they guard against changes in Chromium's GetAIPageContent
// output and in our ConvertAnnotatedPageContentToBlocks conversion.
IN_PROC_BROWSER_TEST_F(AIChatBrowserTest, ExtractsAIPageContent) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://example.com/rewrite.html"),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const std::string content = FetchAIPageContent();
  SCOPED_TRACE(content);

  // The page metadata and readable structure are extracted.
  EXPECT_THAT(content, testing::HasSubstr("PAGE TITLE: Text Rewrite"));
  EXPECT_THAT(content, testing::HasSubstr(
                           "=== PAGE STRUCTURE (XML representation) ==="));
  // The contenteditable's text and both form control values are extracted
  // (apostrophes are XML-escaped in the serialized structure).
  EXPECT_THAT(content, testing::HasSubstr("contenteditable."));
  EXPECT_THAT(content, testing::HasSubstr("<input"));
  EXPECT_THAT(content, testing::HasSubstr("value=\"I&#39;m textarea.\""));
  EXPECT_THAT(content, testing::HasSubstr("value=\"I&#39;m input.\""));

  // ...but the agentic targeting metadata is stripped for content-only
  // extraction: no dom_ids, document identifiers, viewport summary, or the
  // interaction instructions that describe them.
  EXPECT_THAT(content, testing::Not(testing::HasSubstr("dom_id")));
  EXPECT_THAT(content, testing::Not(testing::HasSubstr("document_identifier")));
  EXPECT_THAT(content, testing::Not(testing::HasSubstr("VIEWPORT:")));
  EXPECT_THAT(content,
              testing::Not(testing::HasSubstr("=== INTERACTION INSTRUCTIONS")));
}

// Test for https://github.com/brave/brave-browser/issues/47294
IN_PROC_BROWSER_TEST_F(AIChatBrowserTest,
                       ClosingMultiAssociatedChatDoesNotCrash) {
  auto get_associated_content = [](content::RenderFrameHost* rfh) {
    return AIChatTabHelper::FromWebContents(
        content::WebContents::FromRenderFrameHost(rfh));
  };
  auto* content1 =
      get_associated_content(ui_test_utils::NavigateToURLWithDisposition(
          browser(), GURL("https://example.com/one"),
          WindowOpenDisposition::NEW_FOREGROUND_TAB,
          ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  auto* content2 =
      get_associated_content(ui_test_utils::NavigateToURLWithDisposition(
          browser(), GURL("https://example.com/two"),
          WindowOpenDisposition::NEW_FOREGROUND_TAB,
          ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  auto* content3 =
      get_associated_content(ui_test_utils::NavigateToURLWithDisposition(
          browser(), GURL("https://example.com/three"),
          WindowOpenDisposition::NEW_FOREGROUND_TAB,
          ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  auto* ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(browser()->profile());
  auto* conversation = ai_chat_service->CreateConversation();
  ai_chat_service->MaybeAssociateContent(&content1->web_contents_content(),
                                         conversation->get_conversation_uuid());
  ai_chat_service->MaybeAssociateContent(&content2->web_contents_content(),
                                         conversation->get_conversation_uuid());
  ai_chat_service->MaybeAssociateContent(&content3->web_contents_content(),
                                         conversation->get_conversation_uuid());

  EXPECT_EQ(ai_chat_service->GetInMemoryConversationCountForTesting(), 1u);

  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(base::StrCat(
          {"chrome://leo-ai/", conversation->get_conversation_uuid()})),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  // Close the first tab
  chrome::CloseWindow(browser());
}

}  // namespace ai_chat
