/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_shields/browser/autoplay_whitelist_service.h"
#include "brave/common/brave_paths.h"
#include "brave/vendor/autoplay-whitelist/autoplay_whitelist_parser.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/permission_bubble/mock_permission_prompt_factory.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/dns/mock_host_resolver.h"

const char kVideoPlaying[] = "Video playing";
const char kVideoPlayingDetect[] =
  "window.domAutomationController.send(document.getElementById('status')."
  "textContent);";

class BraveContentSettingsAgentImplAutoplayTest : public InProcessBrowserTest {
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
      embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

      ASSERT_TRUE(embedded_test_server()->Start());

      g_brave_browser_process->autoplay_whitelist_service()->
        autoplay_whitelist_client_->addHost("example.com");
      whitelisted_url_ = embedded_test_server()->GetURL(
        "example.com", "/autoplay/autoplay_by_attr.html");

      user_blocklist_pattern_ =
          ContentSettingsPattern::FromString("http://example.com/*");
    }

    void TearDown() override {
      browser_content_client_.reset();
      content_client_.reset();
    }

    const GURL& whitelisted_url() { return whitelisted_url_; }

    const ContentSettingsPattern& user_blocklist_pattern() {
      return user_blocklist_pattern_;
    }

    HostContentSettingsMap * content_settings() {
      return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    }

    void BlockAutoplay() {
      content_settings()->SetContentSettingCustomScope(
          user_blocklist_pattern_,
          ContentSettingsPattern::Wildcard(),
          ContentSettingsType::AUTOPLAY,
          std::string(),
          CONTENT_SETTING_BLOCK);
    }

    content::WebContents* contents() {
      return browser()->tab_strip_model()->GetActiveWebContents();
    }

    bool NavigateToURLUntilLoadStop(const GURL& url) {
      ui_test_utils::NavigateToURL(browser(), url);
      return WaitForLoadStop(contents());
    }

    void WaitForPlaying() {
      std::string msg_from_renderer;
      ASSERT_TRUE(ExecuteScriptAndExtractString(
                    contents(), "notifyWhenPlaying();",
                    &msg_from_renderer));
      ASSERT_EQ("PLAYING", msg_from_renderer);
    }

 private:
    GURL whitelisted_url_;
    ContentSettingsPattern user_blocklist_pattern_;
    std::unique_ptr<ChromeContentClient> content_client_;
    std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

// Allow autoplay on whitelisted URL by default
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplAutoplayTest,
                       AllowAutoplay) {
  std::string result;
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(whitelisted_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  WaitForPlaying();
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);
}

// Block autoplay, even on whitelisted URL, if user has a blocklist pattern that
// matches the whitelisted URL
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplAutoplayTest,
                       BlockAutoplay) {
  std::string result;
  BlockAutoplay();
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(whitelisted_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);
}
