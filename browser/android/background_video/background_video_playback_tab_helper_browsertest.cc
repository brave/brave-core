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
#include "content/public/test/test_navigation_observer.h"
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

  void InjectScript(const std::u16string& script) {
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    frame->ExecuteJavaScriptForTests(script, base::NullCallback(),
                                     content::ISOLATED_WORLD_ID_GLOBAL);
  }

 protected:
  // Must use HTTPS because `youtube.com` is in Chromium's HSTS preload list.
  net::EmbeddedTestServer https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// TESTS

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       ReplaceExperimentalFlagValues) {
  const GURL url = https_server_.GetURL("youtube.com", "/ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  // Verify the replace method was called exactly 5 times.
  EXPECT_EQ(5, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());

  // Verify all the flags were properly set to `false`.
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

  // Verify the other flags were not modified.
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"another_"
          "flag_for_testing=true\")")
          .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       ReplaceExperimentalFlagValuesInjectedBeforeOnLoad) {
  const GURL url = https_server_.GetURL("youtube.com", "/load_ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  InjectScript(u"window.simulateScriptLoadEvent();");

  EXPECT_EQ(true,
            content::EvalJs(web_contents(), "!!window.ytcfg").ExtractBool());

  // Verify the replace method was called exactly 5 times.
  EXPECT_EQ(5, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());

  // Verify all the flags were properly set to `false`.
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

  // Verify the other flags were not modified.
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"another_"
          "flag_for_testing=true\")")
          .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       DontReplaceExperimentalFlagValuesForSamePageNavigation) {
  const GURL url = https_server_.GetURL("youtube.com", "/ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  ASSERT_TRUE(
      content::EvalJs(web_contents(), "!!document.getElementById('link1')")
          .ExtractBool());
  ASSERT_TRUE(
      content::EvalJs(web_contents(), "!!document.getElementById('link2')")
          .ExtractBool());

  // Verify the replace method was called exactly 5 times.
  EXPECT_EQ(5, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());

  // Navigate to "#section1".
  const GURL url_link1(url.spec() + "#section1");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url_link1,
                                                      1, true);

  // Verify navigation to the same page with the appropriate hash.
  EXPECT_EQ(web_contents()->GetVisibleURL().ref(), "section1");

  // Navigate to "#section2".
  const GURL url_link2(url.spec() + "#section2");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url_link2,
                                                      1, true);

  // Verify navigation.
  EXPECT_EQ(web_contents()->GetVisibleURL().ref(), "section2");

  // Verify the replace method was called exactly 5 times
  // even with multiple same page navigations.
  EXPECT_EQ(5, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());

  // Verify all the flags were properly set to `false`.
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

  // Verify the other flags were not modified.
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"another_"
          "flag_for_testing=true\")")
          .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       OnlyReplaceExperimentalFlagValuesForYouTubeDomains) {
  const GURL url = https_server_.GetURL("youtub.com", "/ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  // Verify nothing is injected when domain does not match.
  EXPECT_EQ(0, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       NoOpIfSerializedExperimentFlagsIsMissing) {
  const GURL url =
      https_server_.GetURL("youtube.com", "/ytcfg_mock_no_flags.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  // Assert that `serializedExperimentFlags` is not present.
  ASSERT_TRUE(
      content::EvalJs(
          web_contents(),
          "typeof "
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags === 'undefined'")
          .ExtractBool());
  // Verify that nothing is injected if `serializedExperimentFlags` is not
  // present.
  EXPECT_EQ(0, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       NoOpIfYtcfgIsMissing) {
  const GURL url = https_server_.GetURL("youtube.com", "/no_ytcfg_mock.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  // Assert that `ytcfg` is not present
  ASSERT_TRUE(
      content::EvalJs(web_contents(), "typeof window.ytcfg === 'undefined'")
          .ExtractBool());
  // Verify that nothing is injected if `ytcfg` is not present.
  EXPECT_EQ(0, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());
}

IN_PROC_BROWSER_TEST_F(AndroidBackgroundVideoPlaybackBrowserTest,
                       IgnoreMissingSerializedExperimentalFlags) {
  const GURL url =
      https_server_.GetURL("youtube.com", "/ytcfg_mock_reduced_flags.html");

  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
  // Assert that some flags were missing by checking
  // `serializedExperimentFlags.includes` returns `false`.
  ASSERT_FALSE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_blocking_document_fullscreen\")")
          .ExtractBool());
  ASSERT_FALSE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_blocking_standard_api\")")
          .ExtractBool());
  ASSERT_FALSE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"html5_"
          "picture_in_picture_logging_onresize\")")
          .ExtractBool());

  // Verify the replace method was called exactly 5 times.
  EXPECT_EQ(5, content::EvalJs(web_contents(), kReplaceCallCount).ExtractInt());

  // Verify the remaining flags were properly set to `false`.
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

  // Verify the other flags were not modified.
  EXPECT_TRUE(
      content::EvalJs(
          web_contents(),
          "window.ytcfg.get(\"WEB_PLAYER_CONTEXT_CONFIGS\").WEB_PLAYER_CONTEXT_"
          "CONFIG_ID_MWEB_WATCH.serializedExperimentFlags.includes(\"another_"
          "flag_for_testing=true\")")
          .ExtractBool());
}
