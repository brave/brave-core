/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/callback.h"
#include "base/path_service.h"
#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/android/android_browser_test.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

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

  // Waits until the document's fullscreen state matches `expected`. Resolves
  // on the 'fullscreenchange' event within a single EvalJs, so no polling and
  // no leftover timeout task.
  bool WaitForFullscreenState(content::WebContents* web_contents,
                              bool expected) {
    return content::EvalJs(web_contents, content::JsReplace(
                                             R"(new Promise(resolve => {
                        const matches = () =>
                            (document.fullscreenElement !== null) === $1;
                        if (matches()) {
                          resolve(true);
                          return;
                        }
                        const listener = () => {
                          if (matches()) {
                            document.removeEventListener('fullscreenchange',
                                                         listener);
                            resolve(true);
                          }
                        };
                        document.addEventListener('fullscreenchange',
                                                  listener);
                      }))",
                                             expected))
        .ExtractBool();
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

  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  helper->MaybeSetFullScreenAndPictureInPictureMode();
  // Wait for the resize to complete triggered by fullscreen change.
  content::WaitForResizeComplete(web_contents());

  // Check the video is in fullscreen mode.
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));
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

  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  helper->MaybeSetFullScreenAndPictureInPictureMode();
  // Wait for the resize to complete triggered by fullscreen change.
  content::WaitForResizeComplete(web_contents());
  EXPECT_TRUE(IsVideoPlaying());

  // Check the video is in fullscreen mode.
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));

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
  ASSERT_TRUE(WaitForFullscreenState(web_contents(), true));

  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  helper->MaybeSetFullScreenAndPictureInPictureMode();

  // Verify that the video is still in fullscreen mode.
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));
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
  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  // Attempt to set fullscreen, which should not change anything.
  helper->MaybeSetFullScreenAndPictureInPictureMode();

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

  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  helper->MaybeSetFullScreenAndPictureInPictureMode();

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
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));
}

// The test is flaky on CI, but succeed on local environment including physical
// device
// TODO(alexeybarabash): https://github.com/brave/brave-browser/issues/48430
// Enable if possible
// Test that MaybeSetFullScreenAndPictureInPictureMode() works with multiple
// calls.
IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       DISABLED_MultipleFullscreenCalls) {
  const GURL url = https_server_.GetURL("youtube.com", "/yt_fullscreen.html");
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);

  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  // First call should trigger fullscreen.
  helper->MaybeSetFullScreenAndPictureInPictureMode();

  // Wait for fullscreen to be triggered.
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));

  // Exit fullscreen to prepare for second test.
  ASSERT_TRUE(content::ExecJs(web_contents(), "document.exitFullscreen()"));
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), false));

  // Second call should work the same as first call.
  helper->MaybeSetFullScreenAndPictureInPictureMode();

  // Wait again for fullscreen to be triggered.
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));
}

// Watching one YouTube video and then opening another is an ordinary flow, not
// an edge case: the user enters fullscreen and PiP on the first video, then
// moves to a second video and expects the button to work there too. This
// verifies the entry point still fires on the new page, so a completed request
// on the previous page leaves no stale state behind that would block the next.
IN_PROC_BROWSER_TEST_F(YouTubeScriptInjectorBrowserTest,
                       WorksAfterNavigatingToAnotherVideo) {
  const GURL url1 = https_server_.GetURL("youtube.com", "/yt_fullscreen.html");
  const GURL url2 =
      https_server_.GetURL("youtube.com", "/yt_fullscreen.html?next=1");

  // The user opens the first video.
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url1, 1,
                                                      true);

  YouTubeScriptInjectorTabHelper* helper = YouTubeScriptInjectorTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents()));
  ASSERT_TRUE(helper);

  // The user triggers fullscreen and PiP on the first video.
  helper->MaybeSetFullScreenAndPictureInPictureMode();
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));

  // Exit fullscreen explicitly so this test does not assert browser fullscreen
  // behavior across navigations.
  ASSERT_TRUE(content::ExecJs(web_contents(), "document.exitFullscreen()"));
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), false));

  // The user moves on to a second, different video.
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url2, 1,
                                                      true);

  // The entry point must work on the second video just as it did on the first.
  helper->MaybeSetFullScreenAndPictureInPictureMode();
  EXPECT_TRUE(WaitForFullscreenState(web_contents(), true));
}
