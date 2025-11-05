/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"

#include <array>
#include <optional>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "leo";
}  // namespace

class PageContentFetcherBrowserTest : public InProcessBrowserTest {
 public:
  PageContentFetcherBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(&https_server_);

    base::FilePath test_data_dir;
    test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
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

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateURL(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(GetActiveWebContents()));
  }

  void FetchPageContent(const base::Location& location,
                        const std::string& expected_text,
                        bool expected_is_video,
                        bool trim_whitespace = true) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    page_content_fetcher_ =
        std::make_unique<ai_chat::PageContentFetcher>(GetActiveWebContents());
    page_content_fetcher_->SetURLLoaderFactoryForTesting(
        shared_url_loader_factory_);
    page_content_fetcher_->FetchPageContent(
        "", base::BindLambdaForTesting([&run_loop, &expected_text,
                                        &expected_is_video, &trim_whitespace](
                                           std::string text, bool is_video,
                                           std::string invalidation_token) {
          EXPECT_EQ(expected_text,
                    trim_whitespace ? base::TrimWhitespaceASCII(
                                          text, base::TrimPositions::TRIM_ALL)
                                    : text);
          EXPECT_EQ(expected_is_video, is_video);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void GetSearchSummarizerKey(const base::Location& location,
                              const std::optional<std::string>& expected_key) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    base::RunLoop run_loop;
    page_content_fetcher_ =
        std::make_unique<ai_chat::PageContentFetcher>(GetActiveWebContents());
    page_content_fetcher_->GetSearchSummarizerKey(base::BindLambdaForTesting(
        [&run_loop, &expected_key](const std::optional<std::string>& key) {
          EXPECT_EQ(expected_key, key);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  std::optional<std::string> GetOpenAIChatButtonNonce() {
    std::optional<std::string> ret_nonce;
    base::RunLoop run_loop;
    page_content_fetcher_ =
        std::make_unique<ai_chat::PageContentFetcher>(GetActiveWebContents());
    page_content_fetcher_->GetOpenAIChatButtonNonce(base::BindLambdaForTesting(
        [&run_loop, &ret_nonce](const std::optional<std::string>& nonce) {
          ret_nonce = nonce;
          run_loop.Quit();
        }));
    run_loop.Run();
    return ret_nonce;
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;

 private:
  std::unique_ptr<ai_chat::PageContentFetcher> page_content_fetcher_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest, FetchPageContent) {
  // Simple page with text
  NavigateURL(https_server_.GetURL("a.com", "/text.html"));
  FetchPageContent(FROM_HERE, "I have spoken", false);
  // Main element
  NavigateURL(https_server_.GetURL("a.com", "/text_with_main.html"));
  FetchPageContent(FROM_HERE, "Only this text", false);
  // Main element with ignored
  NavigateURL(https_server_.GetURL("a.com", "/text_with_main.html"));
  FetchPageContent(FROM_HERE, "Only this text", false);
  // Not a page extraction host and page with no text
  NavigateURL(https_server_.GetURL("a.com", "/canvas.html"));
  FetchPageContent(FROM_HERE, "", false);
}

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest, GetSearchSummarizerKey) {
  // ID and expected result for cases in summarizer_key_meta.html.
  std::vector<std::pair<std::string, std::string>> test_cases = {
      std::make_pair("1", R"({"query":"test","results_hash":"hash"})"),
      std::make_pair("2", R"({"query":"test2","results_hash":"hash"})"),
      std::make_pair("3", R"({"query":"test3","results_hash":"hash"})"),
      std::make_pair("other_attr", R"({"test"})"),
      std::make_pair("plain_string", "plainstring123"),
      std::make_pair("empty_content", ""),
      std::make_pair("empty_content_with_other_attr", ""),
      std::make_pair("no_content", ""),
  };

  std::string remove_script = R"(
    var elements = document.getElementsByName('summarizer-key')
    Array.from(elements).forEach((element) => {
      if (element.getAttribute('id') !== '$1') {
        element.remove();
      }
    })
  )";

  std::string check_script = R"(
    document.getElementsByName('summarizer-key').length === 1 &&
    document.getElementsByName('summarizer-key')[0].getAttribute('id') === '$1'
  )";

  for (const auto& [id, expected_result] : test_cases) {
    SCOPED_TRACE(testing::Message() << "ID: " << id);
    NavigateURL(https_server_.GetURL("a.com", "/summarizer_key_meta.html"));

    ASSERT_TRUE(content::ExecJs(
        GetActiveWebContents()->GetPrimaryMainFrame(),
        base::ReplaceStringPlaceholders(remove_script, {id}, nullptr)));

    EXPECT_TRUE(content::EvalJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                                base::ReplaceStringPlaceholders(check_script,
                                                                {id}, nullptr))
                    .ExtractBool());

    GetSearchSummarizerKey(FROM_HERE, expected_result);
  }
}

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest,
                       GetOpenAIChatButtonNonce) {
  // Test no open Leo button with continue-with-leo ID present.
  GURL url = https_server_.GetURL("a.com", "/open_ai_chat_button.html");
  NavigateURL(url);
  EXPECT_FALSE(GetOpenAIChatButtonNonce());

  // Test valid case.
  NavigateURL(url);
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                              "document.getElementById('valid').setAttribute('"
                              "id', 'continue-with-leo')"));
  EXPECT_EQ(GetOpenAIChatButtonNonce(), "5566");

  // Test invalid cases.
  const auto invalid_cases = std::to_array<std::string_view>(
      {"invalid", "not-a-tag", "no-href", "no-nonce", "empty-nonce",
       "empty-nonce2", "empty-nonce3", "empty-nonce4", "empty-nonce5",
       "empty-nonce6", "not-https-url", "not-search-url", "not-open-leo-url"});

  for (const auto& invalid_case : invalid_cases) {
    SCOPED_TRACE(testing::Message() << "Invalid case: " << invalid_case);
    NavigateURL(url);
    ASSERT_TRUE(content::ExecJs(
        GetActiveWebContents()->GetPrimaryMainFrame(),
        content::JsReplace("document.getElementById($1)."
                           "setAttribute('id', 'continue-with-leo')",
                           invalid_case)));
    EXPECT_FALSE(GetOpenAIChatButtonNonce());
  }
}
