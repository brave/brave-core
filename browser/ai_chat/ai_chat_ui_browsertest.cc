/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "printing/buildflags/buildflags.h"
#include "services/network/public/cpp/network_switches.h"
#include "ui/compositor/compositor_switches.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/browser/printing/test_print_preview_observer.h"
#endif

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "leo";

std::unique_ptr<net::test_server::HttpResponse> HandleSearchQuerySummaryRequest(
    const net::test_server::HttpRequest& request) {
  const GURL url = request.GetURL();
  if (url.path_piece() != "/api/chatllm/raw_data") {
    return nullptr;
  }

  auto query = url.query_piece();
  if (query == "key=%7Btest_key%7D") {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/json");
    response->set_content(
        R"({"conversation": [{"query": "test query",
                                "answer": [{"text": "test summary"}]}]})");
    return response;
  }

  return nullptr;
}

}  // namespace

class AIChatUIBrowserTest : public InProcessBrowserTest {
 public:
  AIChatUIBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(&https_server_);

    base::FilePath test_data_dir;
    test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&HandleSearchQuerySummaryRequest));
    https_server_.StartAcceptingConnections();

    // Set a smaller window size so we can have test data with more pages.
    browser()->window()->SetContentsSize(gfx::Size(800, 600));

    chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebContents(GetActiveWebContents());
    ASSERT_TRUE(chat_tab_helper_);
    chat_tab_helper_->SetUserOptedIn(true);
    OpenAIChatSidePanel();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    ASSERT_TRUE(https_server_.InitializeAndListen());
    // Add a host resolver rule to map all outgoing requests to the test server.
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString() +
            ",EXCLUDE localhost");

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
    command_line->AppendSwitch(::switches::kEnablePixelOutputInTests);
#endif
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  PrefService* prefs() { return browser()->profile()->GetPrefs(); }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateURL(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(GetActiveWebContents()));
  }

  void OpenAIChatSidePanel() {
    auto* side_panel_ui = browser()->GetFeatures().side_panel_ui();
    side_panel_ui->Show(SidePanelEntryId::kChatUI);
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    auto* side_panel = browser_view->unified_side_panel();
    auto* ai_chat_side_panel =
        side_panel->GetViewByID(SidePanelWebUIView::kSidePanelWebViewId);
    ASSERT_TRUE(ai_chat_side_panel);
    auto* side_panel_web_contents =
        (static_cast<views::WebView*>(ai_chat_side_panel))->web_contents();
    ASSERT_TRUE(side_panel_web_contents);
    content::WaitForLoadStop(side_panel_web_contents);
  }

  void FetchPageContent(const base::Location& location,
                        std::string_view expected_text) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    chat_tab_helper_->GetPageContent(
        base::BindLambdaForTesting(
            [&run_loop, expected_text](std::string text, bool is_video,
                                       std::string invalidation_token) {
              EXPECT_FALSE(is_video);
              EXPECT_EQ(text, expected_text);
              run_loop.Quit();
            }),
        "");
    run_loop.Run();
  }

  void FetchSearchQuerySummary(const base::Location& location,
                               const std::optional<ai_chat::SearchQuerySummary>&
                                   expected_search_query_summary) {
    SCOPED_TRACE(testing::Message() << location.ToString());

    base::RunLoop run_loop;
    chat_tab_helper_->MaybeFetchOrClearSearchQuerySummary(
        base::BindLambdaForTesting(
            [&](const std::optional<ai_chat::SearchQuerySummary>&
                    search_query_summary) {
              EXPECT_EQ(search_query_summary, expected_search_query_summary);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<ai_chat::AIChatTabHelper> chat_tab_helper_ = nullptr;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreview) {
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
  FetchPageContent(
      FROM_HERE, "This is the way.\n\nI have spoken.\nWherever I Go, He Goes.");
  // Panel is still active so we don't need to set it up again

  // Page recognition host with a canvas element
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  FetchPageContent(FROM_HERE, "this is the way");
#if BUILDFLAG(IS_WIN)
  // Unsupported locale should return no content for Windows only
  // Other platforms do not use locale for extraction
  const brave_l10n::test::ScopedDefaultLocale locale("xx_XX");
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  FetchPageContent(FROM_HERE, "");
#endif  // #if BUILDFLAG(IS_WIN)
#else
  FetchPageContent(FROM_HERE, "");
#endif

  // Not supported on other hosts
  NavigateURL(https_server_.GetURL("a.com", "/long_canvas.html"));
  FetchPageContent(FROM_HERE, "");
}

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreviewPagesLimit) {
  NavigateURL(
      https_server_.GetURL("docs.google.com", "/extra_long_canvas.html"));
  std::string expected_string(ai_chat::kMaxPreviewPages - 1, '\n');
  base::StrAppend(&expected_string, {"This is the way."});
  FetchPageContent(FROM_HERE, expected_string);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreviewContextLimit) {
  ai_chat::AIChatTabHelper::SetMaxContentLengthForTesting(10);
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  FetchPageContent(FROM_HERE, "This is the way.");

  ai_chat::AIChatTabHelper::SetMaxContentLengthForTesting(20);
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  FetchPageContent(FROM_HERE, "This is the way.\n\nI have spoken.");

  ai_chat::AIChatTabHelper::SetMaxContentLengthForTesting(std::nullopt);
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
// Test print preview extraction while print dialog open
IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintDiaglogExtraction) {
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));

  printing::TestPrintPreviewObserver print_preview_observer(
      /*wait_for_loaded=*/true);
  content::ExecuteScriptAsync(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "window.print();");
  print_preview_observer.WaitUntilPreviewIsReady();
  FetchPageContent(
      FROM_HERE, "This is the way.\n\nI have spoken.\nWherever I Go, He Goes.");
}

// Test print dialog can still be open after print preview extraction
IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, ExtractionPrintDialog) {
  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  FetchPageContent(
      FROM_HERE, "This is the way.\n\nI have spoken.\nWherever I Go, He Goes.");

  printing::TestPrintPreviewObserver print_preview_observer(
      /*wait_for_loaded=*/true);
  content::ExecuteScriptAsync(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "window.print();");
  print_preview_observer.WaitUntilPreviewIsReady();
}

// Disable flaky test on ASAN windows 64-bit
// https://github.com/brave/brave-browser/issues/37969
#if BUILDFLAG(IS_WIN) && defined(ADDRESS_SANITIZER) && defined(ARCH_CPU_64_BITS)
#define MAYBE_PrintPreviewFallback DISABLED_PrintPreviewFallback
#else
#define MAYBE_PrintPreviewFallback PrintPreviewFallback
#endif  // BUILDFLAG(IS_WIN) && defined(ADDRESS_SANITIZER) &&
        // defined(ARCH_CPU_64_BITS)
IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, MAYBE_PrintPreviewFallback) {
  NavigateURL(https_server_.GetURL("a.com", "/text_in_image.pdf"));

  auto run_loop = std::make_unique<base::RunLoop>();
  chat_tab_helper_->GeneratePageContent(
      base::BindLambdaForTesting([&run_loop](std::string text, bool is_video,
                                             std::string invalidation_token) {
        EXPECT_FALSE(is_video);
        EXPECT_EQ(
            text,
            "This is the way.\n\nI have spoken.\nWherever I Go, He Goes.");
        run_loop->Quit();
      }));
  run_loop->Run();
  run_loop = std::make_unique<base::RunLoop>();

  NavigateURL(https_server_.GetURL("a.com", "/canvas.html"));
  chat_tab_helper_->GeneratePageContent(
      base::BindLambdaForTesting([&run_loop](std::string text, bool is_video,
                                             std::string invalidation_token) {
        EXPECT_FALSE(is_video);
        EXPECT_EQ(text, "this is the way");
        run_loop->Quit();
      }));
  run_loop->Run();
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)
#endif  // BUILDFLAG(ENABLE_TEXT_RECOGNITION)

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreviewDisabled) {
  prefs()->SetBoolean(prefs::kPrintPreviewDisabled, true);

  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"));
  FetchPageContent(FROM_HERE, "");
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, FetchSearchQuerySummary_NoMetaTag) {
  // Test when meta tag is not present, should return null result.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  FetchSearchQuerySummary(FROM_HERE, std::nullopt);
  EXPECT_TRUE(chat_tab_helper_->GetConversationHistory().empty());
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_NotBraveSearchSERP) {
  // Test non-brave search SERP URL, should return null result.
  NavigateURL(https_server_.GetURL("brave.com", "/search?q=query"));
  content::ExecuteScriptAsync(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "var meta = document.createElement('meta');"
                              "meta.name = 'summarizer-key';"
                              "meta.content = '{test_key}';"
                              "document.head.appendChild(meta);");
  FetchSearchQuerySummary(FROM_HERE, std::nullopt);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_EmptyMetaTag) {
  // Test empty summarizer-key meta tag, should return null result.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  content::ExecuteScriptAsync(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "document.querySelector('meta[name=summarizer-"
                              "key').content = '';");
  FetchSearchQuerySummary(FROM_HERE, std::nullopt);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_DynamicMetaTag) {
  // Test when summarizer-key meta tag is dynamically inserted, should return
  // the search query summary from the mock response.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  content::ExecuteScriptAsync(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "var meta = document.createElement('meta');"
                              "meta.name = 'summarizer-key';"
                              "meta.content = '{test_key}';"
                              "document.head.appendChild(meta);");
  FetchSearchQuerySummary(
      FROM_HERE, ai_chat::SearchQuerySummary("test query", "test summary"));
}
