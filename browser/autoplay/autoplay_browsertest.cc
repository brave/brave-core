/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/thread_test_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "media/base/media_switches.h"
#include "net/dns/mock_host_resolver.h"

constexpr char kVideoPlaying[] = "Video playing";
constexpr char kVideoPlayingDetect[] =
    "document.getElementById('status').textContent;";
constexpr char kEmbeddedTestServerDirectory[] = "autoplay";

class AutoplayBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    file_autoplay_method_url_ = GURL("file://" + test_data_dir.AsUTF8Unsafe() +
                                     "/autoplay_by_method.html");
    file_autoplay_attr_url_ = GURL("file://" + test_data_dir.AsUTF8Unsafe() +
                                   "/autoplay_by_attr.html");

    index_url_ = embedded_test_server()->GetURL("a.com", "/index.html");
    top_level_page_pattern_ =
        ContentSettingsPattern::FromString(index_url_.spec());
  }

  const GURL& index_url() { return index_url_; }
  const GURL& file_autoplay_method_url() { return file_autoplay_method_url_; }
  const GURL& file_autoplay_attr_url() { return file_autoplay_attr_url_; }

  const ContentSettingsPattern& top_level_page_pattern() {
    return top_level_page_pattern_;
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowAutoplay() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern_, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::AUTOPLAY, CONTENT_SETTING_ALLOW);
  }

  void BlockAutoplay() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern_, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::AUTOPLAY, CONTENT_SETTING_BLOCK);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(contents());
  }

  void GotoAutoplayByAttr(bool muted) {
    if (muted) {
      ASSERT_EQ(true, EvalJs(contents(), "clickAutoplayByAttrMuted()"));
    } else {
      ASSERT_EQ(true, EvalJs(contents(), "clickAutoplayByAttr()"));
    }
    ASSERT_TRUE(WaitForLoadStop(contents()));
    WaitForCanPlay();
  }

  void GotoAutoplayByMethod(bool muted) {
    if (muted) {
      ASSERT_EQ(true, EvalJs(contents(), "clickAutoplayByMethodMuted()"));
    } else {
      ASSERT_EQ(true, EvalJs(contents(), "clickAutoplayByMethod()"));
    }
    ASSERT_TRUE(WaitForLoadStop(contents()));
    WaitForCanPlay();
  }

  void WaitForCanPlay() {
    ASSERT_EQ("CANPLAY", EvalJs(contents(), "notifyWhenCanPlay()"));
  }

 private:
  GURL index_url_;
  GURL file_autoplay_method_url_;
  GURL file_autoplay_attr_url_;
  ContentSettingsPattern top_level_page_pattern_;
};

IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, AllowAutoplay) {
  AllowAutoplay();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByMethod(false);

  EXPECT_EQ(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByAttr(false);
  EXPECT_EQ(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}

// If content setting = BLOCK, ignore play() method call
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, BlockAutoplayByMethod) {
  BlockAutoplay();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByMethod(false);
  // should not play
  EXPECT_NE(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}

// If content setting = BLOCK, ignore autoplay attribute
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, BlockAutoplayByAttribute) {
  BlockAutoplay();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByAttr(false);
  // should not play
  EXPECT_NE(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}

// If content setting = BLOCK, ignore play() method call, even if video would
// play muted.
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, BlockAutoplayByMethodOnMutedVideo) {
  BlockAutoplay();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByMethod(true);
  // should not play
  EXPECT_NE(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}

// If content setting = BLOCK, ignore autoplay attribute, even if the video
// would play muted.
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest,
                       BlockAutoplayByAttributeOnMutedVideo) {
  BlockAutoplay();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByAttr(false);
  // should not play
  EXPECT_NE(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}

class AutoplayNoUserGestureRequiredBrowserTest : public AutoplayBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kNoUserGestureRequiredPolicy);
  }
};

// If content setting = ALLOW, all videos that upstream would autoplay should
// autoplay. (Per new upstream rules, they may play muted by default. This test
// does not verify that.)
IN_PROC_BROWSER_TEST_F(AutoplayNoUserGestureRequiredBrowserTest,
                       AllowAutoplay) {
  AllowAutoplay();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByMethod(false);
  // should play
  EXPECT_EQ(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByAttr(false);
  // should play
  EXPECT_EQ(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}

// Default allow autoplay on file urls
IN_PROC_BROWSER_TEST_F(AutoplayNoUserGestureRequiredBrowserTest, FileAutoplay) {
  NavigateToURLUntilLoadStop(file_autoplay_method_url());
  WaitForCanPlay();
  // should play
  EXPECT_EQ(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));

  NavigateToURLUntilLoadStop(file_autoplay_attr_url());
  WaitForCanPlay();
  // should play
  EXPECT_EQ(kVideoPlaying, EvalJs(contents(), kVideoPlayingDetect));
}
