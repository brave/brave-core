/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
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

const char kVideoPlaying[] = "Video playing";
const char kVideoPlayingDetect[] =
    "window.domAutomationController.send(document.getElementById('status')."
    "textContent);";
const char kEmbeddedTestServerDirectory[] = "autoplay";

class AutoplayBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
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

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
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
        ContentSettingsType::AUTOPLAY, std::string(), CONTENT_SETTING_ALLOW);
  }

  void BlockAutoplay() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern_, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::AUTOPLAY, std::string(), CONTENT_SETTING_BLOCK);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

  void GotoAutoplayByAttr(bool muted) {
    bool clicked;
    if (muted)
      ASSERT_TRUE(ExecuteScriptAndExtractBool(
          contents(),
          "window.domAutomationController.send(clickAutoplayByAttrMuted())",
          &clicked));
    else
      ASSERT_TRUE(ExecuteScriptAndExtractBool(
          contents(),
          "window.domAutomationController.send(clickAutoplayByAttr())",
          &clicked));
    ASSERT_TRUE(WaitForLoadStop(contents()));
    WaitForCanPlay();
    ASSERT_TRUE(clicked);
  }

  void GotoAutoplayByMethod(bool muted) {
    bool clicked;
    if (muted)
      ASSERT_TRUE(ExecuteScriptAndExtractBool(
          contents(),
          "window.domAutomationController.send(clickAutoplayByMethodMuted())",
          &clicked));
    else
      ASSERT_TRUE(ExecuteScriptAndExtractBool(
          contents(),
          "window.domAutomationController.send(clickAutoplayByMethod())",
          &clicked));
    ASSERT_TRUE(WaitForLoadStop(contents()));
    WaitForCanPlay();
    ASSERT_TRUE(clicked);
  }

  void WaitForCanPlay() {
    std::string msg_from_renderer;
    ASSERT_TRUE(ExecuteScriptAndExtractString(
        contents(), "notifyWhenCanPlay();", &msg_from_renderer));
    ASSERT_EQ("CANPLAY", msg_from_renderer);
  }

 private:
  GURL index_url_;
  GURL file_autoplay_method_url_;
  GURL file_autoplay_attr_url_;
  ContentSettingsPattern top_level_page_pattern_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, AllowAutoplay) {
  std::string result;
  AllowAutoplay();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByMethod(false);

  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByAttr(false);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);
}

// If content setting = BLOCK, ignore play() method call
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, BlockAutoplayByMethod) {
  std::string result;
  BlockAutoplay();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByMethod(false);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should not play
  EXPECT_NE(result, kVideoPlaying);
}

// If content setting = BLOCK, ignore autoplay attribute
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, BlockAutoplayByAttribute) {
  std::string result;
  BlockAutoplay();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByAttr(false);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should not play
  EXPECT_NE(result, kVideoPlaying);
}

// If content setting = BLOCK, ignore play() method call, even if video would
// play muted.
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest, BlockAutoplayByMethodOnMutedVideo) {
  std::string result;
  BlockAutoplay();

  NavigateToURLUntilLoadStop(index_url());
  GotoAutoplayByMethod(true);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should not play
  EXPECT_NE(result, kVideoPlaying);
}

// If content setting = BLOCK, ignore autoplay attribute, even if the video
// would play muted.
IN_PROC_BROWSER_TEST_F(AutoplayBrowserTest,
                       BlockAutoplayByAttributeOnMutedVideo) {
  std::string result;
  BlockAutoplay();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByAttr(false);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should not play
  EXPECT_NE(result, kVideoPlaying);
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
  std::string result;
  AllowAutoplay();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByMethod(false);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should play
  EXPECT_EQ(result, kVideoPlaying);

  result.clear();

  ASSERT_TRUE(NavigateToURLUntilLoadStop(index_url()));
  GotoAutoplayByAttr(false);
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should play
  EXPECT_EQ(result, kVideoPlaying);
}

// Default allow autoplay on file urls
IN_PROC_BROWSER_TEST_F(AutoplayNoUserGestureRequiredBrowserTest, FileAutoplay) {
  std::string result;

  NavigateToURLUntilLoadStop(file_autoplay_method_url());
  WaitForCanPlay();
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should play
  EXPECT_EQ(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(file_autoplay_attr_url());
  WaitForCanPlay();
  EXPECT_TRUE(
      ExecuteScriptAndExtractString(contents(), kVideoPlayingDetect, &result));
  // should play
  EXPECT_EQ(result, kVideoPlaying);
}
