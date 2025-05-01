/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/android/android_browser_test.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {
constexpr char kReplaceCallCount[] = "window.getReplaceCallCount()";
}  // namespace

class AndroidBackgroundVideoPlaybackBrowserTest : public PlatformBrowserTest {
 public:
  AndroidBackgroundVideoPlaybackBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~AndroidBackgroundVideoPlaybackBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir = GetTestDataDir();

    https_server_.ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(&https_server_);
    ASSERT_TRUE(https_server_.Start());
  }

  base::FilePath GetTestDataDir() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathService::CheckedGet(brave::DIR_TEST_DATA);
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 protected:
  // Must use HTTPS because `youtube.com` is in Chromium's HSTS preload list.
  net::EmbeddedTestServer https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// TESTS

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       TestInjectionMatch) {
  const GURL url = https_server_.GetURL("youtube.com", "/ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  EXPECT_EQ(5, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());

  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_blocking_ontimeupdate=false\")")
          .ExtractBool());
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_blocking_onresize=false\")")
          .ExtractBool());
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_blocking_document_fullscreen=false\")")
          .ExtractBool());
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_blocking_standard_api=false\")")
          .ExtractBool());
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_logging_onresize=false\")")
          .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       TestInjectionNoMatch) {
  const GURL url = https_server_.GetURL("youtub.com", "/ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  EXPECT_EQ(0, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());
}
