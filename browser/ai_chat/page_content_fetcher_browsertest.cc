/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/compositor/compositor_switches.h"

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

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
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

  void NavigateURL(const GURL& url) {
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(contents));
  }

  void FetchPageContent(const std::string& expected_text,
                        bool expected_is_video) {
    base::RunLoop run_loop;
    ai_chat::FetchPageContent(
        browser()->tab_strip_model()->GetActiveWebContents(),
        base::BindLambdaForTesting([&run_loop, &expected_text,
                                    &expected_is_video](std::string text,
                                                        bool is_video) {
          ASSERT_EQ(expected_text, base::TrimWhitespaceASCII(
                                       text, base::TrimPositions::TRIM_ALL));
          ASSERT_EQ(expected_is_video, is_video);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest,
                       FetchPageContentWithText) {
  NavigateURL(https_server_.GetURL("a.com", "/text.html"));
  FetchPageContent("I have spoken", false);
}

IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest, FetchPageContentNoText) {
  NavigateURL(https_server_.GetURL("a.com", "/canvas.html"));
  FetchPageContent("", false);
}

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
IN_PROC_BROWSER_TEST_F(PageContentFetcherBrowserTest,
                       FetchPageContentViaTextExtraction) {
  NavigateURL(https_server_.GetURL("docs.google.com", "/canvas.html"));
  FetchPageContent("this is the way", false);
}
#endif
