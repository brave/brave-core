/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/callback.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/android/android_browser_test.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const base::TimeDelta kCheckFrequency = base::Milliseconds(200);
const base::TimeDelta kTaskTimeout = base::Seconds(10);

constexpr std::u16string_view kSimulateDelayedScriptLoad =
    uR"(
  // Replace the contents of #movie_player with video and button.
  const moviePlayer = document.getElementById('movie_player');
  if (moviePlayer) {
    moviePlayer.innerHTML = `
      <video class="html5-main-video" src="mov_bbb.mp4" controls
      onclick="requestFullscreen();"></video>
      <button class="fullscreen-icon"
      onclick="document.querySelector('video.html5-main-video')
        .requestFullscreen();">⛶</button>
    `;
  })";

}  // namespace

class YouTubeScriptInjectorBrowserTest : public PlatformBrowserTest {
 public:
  YouTubeScriptInjectorBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~YouTubeScriptInjectorBrowserTest() override = default;

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

  void InjectScript(const std::u16string_view script) {
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    frame->ExecuteJavaScriptForTests(std::u16string(script),
                                     base::NullCallback(),
                                     content::ISOLATED_WORLD_ID_GLOBAL);
  }

  bool WaitForJsResult(content::WebContents* web_contents,
                       const std::string& script) {
    base::RunLoop run_loop;
    bool fulfilled = false;
    bool timed_out = false;

    auto check = std::make_unique<base::RepeatingClosure>();
    *check = base::BindRepeating(
        [](base::RepeatingClosure* check, base::RunLoop* run_loop,
           content::WebContents* web_contents, const std::string& script,
           bool* fulfilled) {
          if (content::EvalJs(web_contents, script).ExtractBool()) {
            *fulfilled = true;
            run_loop->Quit();
          } else {
            base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
                FROM_HERE, base::BindOnce((*check)), kCheckFrequency);
          }
        },
        check.get(), &run_loop, web_contents, script, &fulfilled);

    // Start the first check.
    (*check).Run();

    // Set up timeout.
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(
            [](base::RunLoop* run_loop, bool* timed_out) {
              *timed_out = true;
              run_loop->Quit();
            },
            &run_loop, &timed_out),
        kTaskTimeout);

    run_loop.Run();

    return fulfilled && !timed_out;
  }

  bool IsVideoPlaying() {
    return content::EvalJs(web_contents(),
                           "document.querySelector('video.html5-main-video')."
                           "paused === false")
        .ExtractBool();
  }

 protected:
  // Must use HTTPS because `youtube.com` is in Chromium's HSTS preload list.
  net::EmbeddedTestServer https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       SetFullscreenFromPausedVideo) {
  const GURL url = https_server_.GetURL("youtube.com", "/yt_fullscreen.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  // Video is not playing initially.
  ASSERT_FALSE(IsVideoPlaying());

  // The document is not in fullscreen mode.
  ASSERT_TRUE(
      content::EvalJs(web_contents(), "document.fullscreenElement === null")
          .ExtractBool());

  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  helper->MaybeSetFullscreen();
  // Wait for the resize to complete triggered by fullscreen change.
  content::WaitForResizeComplete(web_contents());

  // Check the video is in fullscreen mode.
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));
}

IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       SetFullscreenFromPlayingVideo) {
  const GURL url = https_server_.GetURL("youtube.com", "/yt_fullscreen.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  // The document is not in fullscreen mode.
  ASSERT_TRUE(
      content::EvalJs(web_contents(), "document.fullscreenElement === null")
          .ExtractBool());

  // Simulate a playing video before entering fullscreen.
  ASSERT_TRUE(
      content::EvalJs(
          web_contents(),
          "(async () => {"
          "await document.querySelector('video.html5-main-video').play();"
          "return true;"
          "})();")
          .ExtractBool());

  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  helper->MaybeSetFullscreen();
  // Wait for the resize to complete triggered by fullscreen change.
  content::WaitForResizeComplete(web_contents());
  EXPECT_TRUE(IsVideoPlaying());

  // Check the video is in fullscreen mode.
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));

  // Check again the video is still playing.
  EXPECT_TRUE(IsVideoPlaying());
}

IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       NoOpIfAlreadyInFullscreen) {
  const GURL url = https_server_.GetURL("youtube.com", "/yt_fullscreen.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  // Video is not playing.
  ASSERT_FALSE(IsVideoPlaying());
  // Simulate fullscreen button click to enter fullscreen mode.
  ASSERT_TRUE(content::ExecJs(
      web_contents(),
      "document.querySelector('button.fullscreen-icon').click()"));
  // Wait for the resize to complete triggered by fullscreen change.
  content::WaitForResizeComplete(web_contents());
  // Assert the video is in fullscreen mode.
  ASSERT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));

  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  helper->MaybeSetFullscreen();

  // Verify that the video is still in fullscreen mode.
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));
}

IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       NoOpIfPlayerIsNotFound) {
  // Load a page without a video.
  const GURL url = https_server_.GetURL("youtube.com", "/ytcftg_mock.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  // Assert that movie player is not present.
  ASSERT_TRUE(
      content::EvalJs(web_contents(),
                      "document.getElementById('movie_player') === null")
          .ExtractBool());

  std::string dom_before =
      content::EvalJs(web_contents(), "document.body.innerHTML")
          .ExtractString();
  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  // Attempt to set fullscreen, which should not change anything.
  helper->MaybeSetFullscreen();

  std::string dom_after =
      content::EvalJs(web_contents(), "document.body.innerHTML")
          .ExtractString();
  // Assert that the DOM remains unchanged.
  EXPECT_EQ(dom_before, dom_after);
}

// Simulates the real-world scenario where the fullscreen injection
// script runs before YouTube's player is fully initialized. The test verifies
// that the mutation observer correctly waits for and detects when all key
// elements become available. kSimulateDelayedScriptLoad constant artificially
// recreates a delay by injecting the video and button elements after the
// fullscreen script has already started observing, mimicking YouTube's actual
// loading behavior.
IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       SetFullscreenOnElementsLoadingDelayed) {
  const GURL url =
      https_server_.GetURL("youtube.com", "/yt_fullscreen_delayed.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  // Simulate async video loading by presenting a player ('movie_player') that
  // is not immediately available. Assert that movie player is present.
  ASSERT_TRUE(
      content::EvalJs(web_contents(),
                      "document.getElementById('movie_player') !== null")
          .ExtractBool());

  // Assert that the video element (inside the player) is not present initially.
  ASSERT_TRUE(content::EvalJs(
                  web_contents(),
                  "document.querySelector('video.html5-main-video') === null")
                  .ExtractBool());
  // Assert that the button (inside the player) to trigger fullscreen is not
  // present initially.
  ASSERT_TRUE(content::EvalJs(
                  web_contents(),
                  "document.querySelector('button.fullscreen-icon') === null")
                  .ExtractBool());

  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  helper->MaybeSetFullscreen();

  // Inject a script to simulate delayed loading of the video element fullscreen
  // button.
  InjectScript(kSimulateDelayedScriptLoad);
  // Assert that the video element is now present.
  ASSERT_TRUE(content::EvalJs(
                  web_contents(),
                  "document.querySelector('video.html5-main-video') !== null")
                  .ExtractBool());
  // Assert that the button to trigger fullscreen is now present.
  ASSERT_TRUE(content::EvalJs(
                  web_contents(),
                  "document.querySelector('button.fullscreen-icon') !== null")
                  .ExtractBool());

  // Wait for the mutation observer to complete and trigger fullscreen mode.
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));
}

// The test is flaky on CI, but succeed on local environment including physical
// device
// TODO(alexeybarabash): https://github.com/brave/brave-browser/issues/48430
// Enable if possible
// Test that MaybeSetFullscreen() works with multiple calls.
IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       MultipleFullscreenCalls) {
  const GURL url = https_server_.GetURL("youtube.com", "/yt_fullscreen.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  // Verify initially no fullscreen request has been made.
  EXPECT_FALSE(helper->HasFullscreenBeenRequested());

  // First call should trigger fullscreen.
  helper->MaybeSetFullscreen();
  EXPECT_TRUE(helper->HasFullscreenBeenRequested());

  // Wait for fullscreen to be triggered.
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));

  // Exit fullscreen to prepare for second test.
  ASSERT_TRUE(content::ExecJs(web_contents(), "document.exitFullscreen()"));
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement === null"));

  // Second call should work the same as first call.
  helper->MaybeSetFullscreen();

  // Should still show fullscreen was requested a second time for this page.
  EXPECT_TRUE(helper->HasFullscreenBeenRequested());

  // Wait again for fullscreen to be triggered.
  EXPECT_TRUE(
      WaitForJsResult(web_contents(), "document.fullscreenElement !== null"));
}

// Test that fullscreen state resets on navigation.
IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       FullscreenStateResetsOnNavigation) {
  const GURL url1 = https_server_.GetURL("youtube.com", "/watch?v=something");
  const GURL url2 = https_server_.GetURL("youtube.com", "/watch?v=different");

  // Navigate to first YouTube page.
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url1, 1,
                                                      true);

  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(helper);

  // Verify initially no fullscreen request has been made.
  EXPECT_FALSE(helper->HasFullscreenBeenRequested());

  // Make fullscreen request on first page.
  helper->MaybeSetFullscreen();
  EXPECT_TRUE(helper->HasFullscreenBeenRequested());

  // Navigate to second YouTube page
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url2, 1,
                                                      true);

  // Verify fullscreen state resets on navigation.
  EXPECT_FALSE(helper->HasFullscreenBeenRequested());

  // Verify MaybeSetFullscreen() works again on new page.
  helper->MaybeSetFullscreen();
  EXPECT_TRUE(helper->HasFullscreenBeenRequested());
}
