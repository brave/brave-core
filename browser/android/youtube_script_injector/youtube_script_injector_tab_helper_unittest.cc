/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace youtube_script_injector {

class YouTubeScriptInjectorTabHelperTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    YouTubeScriptInjectorTabHelper::CreateForWebContents(web_contents());
  }

  YouTubeScriptInjectorTabHelper* GetHelper() {
    return YouTubeScriptInjectorTabHelper::FromWebContents(web_contents());
  }

  void NavigateToURL(const GURL& url) {
    content::WebContentsTester::For(web_contents())->NavigateAndCommit(url);
  }
};

TEST_F(YouTubeScriptInjectorTabHelperTest, InvalidOrEmptyUrl) {
  NavigateToURL(GURL());
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL(""));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("not a url"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest, NonYouTubeDomain) {
  NavigateToURL(GURL("https://vimeo.com/watch?v=abcdefg"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://example.com/watch?v=abcdefg"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest, YouTubeDomainWrongPath) {
  NavigateToURL(GURL("https://www.youtube.com/other?v=abcdefg"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://www.youtube.com/watchlater?v=abcdefg"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());

  // Path is case-sensitive.
  NavigateToURL(GURL("https://www.youtube.com/Watch?v=abcdefg"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathNoQuery) {
  NavigateToURL(GURL("https://www.youtube.com/watch"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathNoVParam) {
  NavigateToURL(GURL("https://www.youtube.com/watch?foo=bar"));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest,
       YouTubeDomainCorrectPathEmptyVParam) {
  NavigateToURL(GURL("https://www.youtube.com/watch?v="));
  EXPECT_FALSE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest,
       YouTubeDomainCorrectPathValidVParam) {
  NavigateToURL(GURL("https://www.youtube.com/watch?v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://youtube.com/watch?v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://m.youtube.com/watch?v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://www.youtube.com/watch?v=abcdefg&foo=bar"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://www.youtube.com/watch?foo=bar&v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  // First v param wins.
  NavigateToURL(GURL("https://www.youtube.com/watch?v=abcdefg&v=1234567"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest,
       YouTubeDomainCorrectPathWhitespaceVParam) {
  NavigateToURL(GURL("https://www.youtube.com/watch?v= abcdefg "));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest,
       YouTubeDomainCorrectPathCaseInsensitive) {
  NavigateToURL(GURL("https://www.youtube.com/watch?v=ABCdefG"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());
}

TEST_F(YouTubeScriptInjectorTabHelperTest, YouTubeDomainCorrectPathSubdomain) {
  NavigateToURL(GURL("https://music.youtube.com/watch?v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://gaming.youtube.com/watch?v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());

  NavigateToURL(GURL("https://m.youtube.com/watch?v=abcdefg"));
  EXPECT_TRUE(GetHelper()->IsYouTubeVideo());
}

// Test fullscreen state management with PageUserData.
TEST_F(YouTubeScriptInjectorTabHelperTest, FullscreenStateManagement) {
  // Navigate to a YouTube video.
  NavigateToURL(GURL("https://www.youtube.com/watch?v=abcdefg"));

  // Initially, no fullscreen request should be recorded.
  EXPECT_FALSE(GetHelper()->HasFullscreenBeenRequested());

  // Set fullscreen requested.
  GetHelper()->SetFullscreenRequested(true);
  EXPECT_TRUE(GetHelper()->HasFullscreenBeenRequested());

  // Unset fullscreen requested.
  GetHelper()->SetFullscreenRequested(false);
  EXPECT_FALSE(GetHelper()->HasFullscreenBeenRequested());

  // Set it back to true.
  GetHelper()->SetFullscreenRequested(true);
  EXPECT_TRUE(GetHelper()->HasFullscreenBeenRequested());
}

// Test fullscreen state resets on navigation.
TEST_F(YouTubeScriptInjectorTabHelperTest, FullscreenStateResetsOnNavigation) {
  // Navigate to first YouTube video.
  NavigateToURL(GURL("https://www.youtube.com/watch?v=abcdefg"));

  // Set fullscreen requested for first page.
  GetHelper()->SetFullscreenRequested(true);
  EXPECT_TRUE(GetHelper()->HasFullscreenBeenRequested());

  // Navigate to second YouTube video.
  NavigateToURL(GURL("https://www.youtube.com/watch?v=1234567"));

  // State should reset for new page.
  EXPECT_FALSE(GetHelper()->HasFullscreenBeenRequested());

  // Should be able to set fullscreen for new page.
  GetHelper()->SetFullscreenRequested(true);
  EXPECT_TRUE(GetHelper()->HasFullscreenBeenRequested());
}

}  // namespace youtube_script_injector
