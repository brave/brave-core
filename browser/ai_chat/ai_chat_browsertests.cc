/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AiChatBrowserTest : public InProcessBrowserTest {
 public:
  AiChatBrowserTest() = default;

  void SetUpOnMainThread() override {
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir.AppendASCII("ai_chat"));
    https_server_.StartAcceptingConnections();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
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
    std::string content;
    base::RunLoop run_loop;
    page_content_fetcher_ = std::make_unique<PageContentFetcher>(
        browser()->tab_strip_model()->GetActiveWebContents());
    page_content_fetcher_->FetchPageContent(
        "", base::BindLambdaForTesting(
                [&run_loop, &content](std::string page_content, bool is_video,
                                      std::string invalidation_token) {
                  content = std::move(page_content);
                  run_loop.Quit();
                }));
    run_loop.Run();
    return content;
  }

 private:
  std::unique_ptr<PageContentFetcher> page_content_fetcher_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(AiChatBrowserTest, YoutubeNavigations) {
  const GURL url("https://www.youtube.com/youtube.html");

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  const std::string initial_content = FetchPageContent();
  EXPECT_EQ("Initial content", initial_content);

  constexpr const char kClick[] =
      R"js(
        document.getElementById('config').click()
      )js";

  content::WebContentsConsoleObserver console_observer(ActiveWebContents());
  console_observer.SetPattern("CLICKED");

  EXPECT_TRUE(content::ExecJs(ActiveWebContents(), kClick,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS));
  EXPECT_TRUE(console_observer.Wait());

  const std::string intercepted_content = FetchPageContent();
  EXPECT_EQ("Intercepted content", intercepted_content);
}

}  // namespace ai_chat
