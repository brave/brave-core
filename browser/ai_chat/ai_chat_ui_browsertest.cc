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
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/pdf/pdf_extension_test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
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
  } else if (query == "key=multi") {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/json");
    response->set_content(
        R"({"conversation": [{"query": "test query",
                                "answer": [{"text": "test summary"}]},
                              {"query": "test query 2",
                                "answer": [{"text": "test summary 2"}]}]})");
    return response;
  }

  return nullptr;
}

}  // namespace

class AIChatUIBrowserTest : public InProcessBrowserTest {
 public:
  AIChatUIBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
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
    ai_chat::SetUserOptedIn(prefs(), true);
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

  void NavigateURL(const GURL& url, bool wait_for_loaded = true) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    if (wait_for_loaded) {
      ASSERT_TRUE(WaitForLoadStop(GetActiveWebContents()));
    }
  }

  void OpenAIChatSidePanel() {
    auto* side_panel_ui = browser()->GetFeatures().side_panel_ui();
    side_panel_ui->Show(SidePanelEntryId::kChatUI);
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    auto* side_panel = browser_view->contents_height_side_panel();
    auto* ai_chat_side_panel =
        side_panel->GetViewByID(SidePanelWebUIView::kSidePanelWebViewId);
    ASSERT_TRUE(ai_chat_side_panel);
    auto* side_panel_web_contents =
        (static_cast<views::WebView*>(ai_chat_side_panel))->web_contents();
    ASSERT_TRUE(side_panel_web_contents);
    content::WaitForLoadStop(side_panel_web_contents);
  }

  void FetchPageContent(const base::Location& location,
                        std::string_view expected_text,
                        bool wait_for_callback = true) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    chat_tab_helper_->GetContent(base::BindLambdaForTesting(
        [&run_loop, expected_text,
         wait_for_callback](ai_chat::PageContent content) {
          EXPECT_FALSE(content.is_video);
          EXPECT_EQ(content.content, expected_text);
          if (wait_for_callback) {
            run_loop.Quit();
          }
        }));
    if (wait_for_callback) {
      run_loop.Run();
    }
  }

  void FetchSearchQuerySummary(
      const base::Location& location,
      const std::optional<std::vector<ai_chat::SearchQuerySummary>>&
          expected_search_query_summary) {
    SCOPED_TRACE(testing::Message() << location.ToString());

    base::RunLoop run_loop;
    chat_tab_helper_->GetStagedEntriesFromContent(base::BindLambdaForTesting(
        [&](const std::optional<std::vector<ai_chat::SearchQuerySummary>>&
                search_query_summary) {
          EXPECT_EQ(search_query_summary, expected_search_query_summary);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  bool HasPendingGetContentRequest() {
    return chat_tab_helper_->pending_get_page_content_callback_ ? true : false;
  }

  std::optional<std::vector<ai_chat::mojom::UploadedFilePtr>>
  GetScreenshotsSync() {
    base::test::TestFuture<
        std::optional<std::vector<ai_chat::mojom::UploadedFilePtr>>>
        future;
    chat_tab_helper_->GetScreenshots(future.GetCallback());
    return future.Take();
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<ai_chat::AIChatTabHelper, DanglingUntriaged> chat_tab_helper_ =
      nullptr;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};


IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PrintPreviewDisabled) {
  prefs()->SetBoolean(prefs::kPrintPreviewDisabled, true);

  NavigateURL(https_server_.GetURL("docs.google.com", "/long_canvas.html"),
              false);
  FetchPageContent(FROM_HERE, "");
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, FetchSearchQuerySummary_NoMetaTag) {
  // Test when meta tag is not present, should return null result.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  FetchSearchQuerySummary(FROM_HERE, std::nullopt);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, FetchPageContentPDF) {
  constexpr char kExpectedText[] = "This is the way\nI have spoken";
  NavigateURL(https_server_.GetURL("a.com", "/dummy.pdf"));
  ASSERT_TRUE(
      pdf_extension_test_util::EnsurePDFHasLoaded(GetActiveWebContents()));
  FetchPageContent(FROM_HERE, kExpectedText);

  NavigateURL(https_server_.GetURL("a.com", "/empty_pdf.pdf"));
  ASSERT_TRUE(
      pdf_extension_test_util::EnsurePDFHasLoaded(GetActiveWebContents()));
  FetchPageContent(FROM_HERE, "");

  // Test pdf tab loaded in background.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), https_server_.GetURL("a.com", "/dummy.pdf"),
      WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_EQ(1, browser()->tab_strip_model()->active_index());
  ASSERT_TRUE(
      pdf_extension_test_util::EnsurePDFHasLoaded(GetActiveWebContents()));
  // It needs update because it was created for tab 0 at setup
  chat_tab_helper_ =
      ai_chat::AIChatTabHelper::FromWebContents(GetActiveWebContents());
  ASSERT_TRUE(chat_tab_helper_);
  FetchPageContent(FROM_HERE, kExpectedText);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_NotBraveSearchSERP) {
  // Test non-brave search SERP URL, should return null result.
  NavigateURL(https_server_.GetURL("brave.com", "/search?q=query"));
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "var meta = document.createElement('meta');"
                              "meta.name = 'summarizer-key';"
                              "meta.content = '{test_key}';"
                              "document.head.appendChild(meta);"));
  FetchSearchQuerySummary(FROM_HERE, std::nullopt);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_EmptyMetaTag) {
  // Test empty summarizer-key meta tag, should return null result.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "var meta = document.createElement('meta');"
                              "meta.name = 'summarizer-key';"
                              "meta.content = '';"
                              "document.head.appendChild(meta);"));
  FetchSearchQuerySummary(FROM_HERE, std::nullopt);
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_DynamicMetaTag_SingleQuery) {
  // Test when summarizer-key meta tag is dynamically inserted, should return
  // the search query summary from the mock response.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "var meta = document.createElement('meta');"
                              "meta.name = 'summarizer-key';"
                              "meta.content = '{test_key}';"
                              "document.head.appendChild(meta);"));
  FetchSearchQuerySummary(FROM_HERE, std::vector<ai_chat::SearchQuerySummary>(
                                         {{"test query", "test summary"}}));

  ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "document.querySelector('meta[name=summarizer-"
                              "key').content = 'multi';"));
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest,
                       FetchSearchQuerySummary_DynamicMetaTag_MultiQuery) {
  // Test when summarizer-key meta tag is dynamically inserted, should return
  // the search query summary from the mock response.
  NavigateURL(https_server_.GetURL("search.brave.com", "/search?q=query"));
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "var meta = document.createElement('meta');"
                              "meta.name = 'summarizer-key';"
                              "meta.content = 'multi';"
                              "document.head.appendChild(meta);"));

  FetchSearchQuerySummary(FROM_HERE, std::vector<ai_chat::SearchQuerySummary>(
                                         {{"test query", "test summary"},
                                          {"test query 2", "test summary 2"}}));
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, PdfScreenshot) {
  NavigateURL(https_server_.GetURL("a.com", "/text_in_image.pdf"));
  ASSERT_TRUE(
      pdf_extension_test_util::EnsurePDFHasLoaded(GetActiveWebContents()));

  auto result = GetScreenshotsSync();
  ASSERT_TRUE(result);
  EXPECT_EQ(result->size(), 4u);
  EXPECT_TRUE(std::ranges::any_of(
      result.value(), [](const auto& entry) { return !entry->data.empty(); }));
}

IN_PROC_BROWSER_TEST_F(AIChatUIBrowserTest, WebContentsShouldBeFocused) {
  auto* side_panel_ui = browser()->GetFeatures().side_panel_ui();
  side_panel_ui->Show(SidePanelEntryId::kChatUI);
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* side_panel = browser_view->contents_height_side_panel();
  auto* ai_chat_side_panel = static_cast<views::WebView*>(
      side_panel->GetViewByID(SidePanelWebUIView::kSidePanelWebViewId));
  ASSERT_TRUE(ai_chat_side_panel);

  auto* side_panel_web_contents = ai_chat_side_panel->web_contents();
  ASSERT_TRUE(side_panel_web_contents);

  const auto has_focus = content::EvalJs(
      side_panel_web_contents->GetPrimaryMainFrame(), "document.hasFocus()");
  EXPECT_TRUE(has_focus.ExtractBool());
}
