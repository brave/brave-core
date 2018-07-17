/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
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

namespace {

class PageReloadWaiter {
 public:
  explicit PageReloadWaiter(content::WebContents* web_contents)
      : web_contents_(web_contents),
        navigation_observer_(web_contents,
                             web_contents->GetLastCommittedURL()) {}

  bool Wait() {
    navigation_observer_.WaitForNavigationFinished();
    return content::WaitForLoadStop(web_contents_);
  }

 private:
  content::WebContents* web_contents_;
  content::TestNavigationManager navigation_observer_;
};

}  // namespace

class AutoplayPermissionContextBrowserTest : public InProcessBrowserTest {
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
      PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
      test_data_dir = test_data_dir.AppendASCII("autoplay");
      embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

      ASSERT_TRUE(embedded_test_server()->Start());

      autoplay_method_url_ =
        embedded_test_server()->GetURL("a.com", "/autoplay_by_method.html");
      autoplay_attr_url_ =
        embedded_test_server()->GetURL("a.com", "/autoplay_by_attr.html");
      autoplay_method_muted_url_ =
        embedded_test_server()->GetURL("a.com", "/autoplay_by_method_muted.html");
      autoplay_attr_muted_url_ =
        embedded_test_server()->GetURL("a.com", "/autoplay_by_attr_muted.html");
      file_autoplay_method_url_ =
        GURL("file://" + test_data_dir.AsUTF8Unsafe() +
             "/autoplay_by_method.html");
      file_autoplay_attr_url_ =
        GURL("file://" + test_data_dir.AsUTF8Unsafe() +
             "/autoplay_by_attr.html");

      GURL pattern_url = embedded_test_server()->GetURL("a.com", "/index.html");
      top_level_page_pattern_ =
          ContentSettingsPattern::FromString(pattern_url.spec());
    }

    void TearDown() override {
      browser_content_client_.reset();
      content_client_.reset();
    }

    const GURL& autoplay_method_url() { return autoplay_method_url_; }
    const GURL& autoplay_attr_url() { return autoplay_attr_url_; }
    const GURL& autoplay_method_muted_url() { return autoplay_method_muted_url_; }
    const GURL& autoplay_attr_muted_url() { return autoplay_attr_muted_url_; }
    const GURL& file_autoplay_method_url() { return file_autoplay_method_url_; }
    const GURL& file_autoplay_attr_url() { return file_autoplay_attr_url_; }

    const ContentSettingsPattern& top_level_page_pattern() {
      return top_level_page_pattern_;
    }

    HostContentSettingsMap * content_settings() {
      return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    }

    void AllowAutoplay() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern_,
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_AUTOPLAY,
          std::string(),
          CONTENT_SETTING_ALLOW);
    }

    void BlockAutoplay() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern_,
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_AUTOPLAY,
          std::string(),
          CONTENT_SETTING_BLOCK);
    }

    void ResetAutoplayToDefault() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern_,
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_AUTOPLAY,
          std::string(),
          CONTENT_SETTING_DEFAULT);
    }

    content::WebContents* contents() {
      return browser()->tab_strip_model()->GetActiveWebContents();
    }

    bool NavigateToURLUntilLoadStop(const GURL& url) {
      ui_test_utils::NavigateToURL(browser(), url);
      return WaitForLoadStop(contents());
    }

  private:
    GURL autoplay_method_url_;
    GURL autoplay_attr_url_;
    GURL autoplay_method_muted_url_;
    GURL autoplay_attr_muted_url_;
    GURL file_autoplay_method_url_;
    GURL file_autoplay_attr_url_;
    ContentSettingsPattern top_level_page_pattern_;
    std::unique_ptr<ChromeContentClient> content_client_;
    std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

// Autoplay is ask by default
IN_PROC_BROWSER_TEST_F(AutoplayPermissionContextBrowserTest, AskByDefault) {
  std::string result;
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());

  NavigateToURLUntilLoadStop(autoplay_method_url());
  EXPECT_TRUE(manager->IsBubbleVisible());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(autoplay_attr_url());
  EXPECT_TRUE(manager->IsBubbleVisible());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  // Muted version of above
  result.clear();

  NavigateToURLUntilLoadStop(autoplay_method_muted_url());
  EXPECT_TRUE(manager->IsBubbleVisible());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(autoplay_attr_muted_url());
  EXPECT_TRUE(manager->IsBubbleVisible());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);
}

// Click allow from promt
IN_PROC_BROWSER_TEST_F(AutoplayPermissionContextBrowserTest, ClickAllow) {
  std::string result;
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  popup_prompt_factory->set_response_type(PermissionRequestManager::ACCEPT_ALL);

  NavigateToURLUntilLoadStop(autoplay_method_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  ResetAutoplayToDefault();
  popup_prompt_factory->ResetCounts();
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_attr_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  // Muted version of above
  ResetAutoplayToDefault();
  popup_prompt_factory->ResetCounts();
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_method_muted_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  ResetAutoplayToDefault();
  popup_prompt_factory->ResetCounts();
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_attr_muted_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);
}

// Click block from promt
IN_PROC_BROWSER_TEST_F(AutoplayPermissionContextBrowserTest, ClickBlock) {
  std::string result;
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  popup_prompt_factory->set_response_type(PermissionRequestManager::DENY_ALL);

  NavigateToURLUntilLoadStop(autoplay_method_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  ResetAutoplayToDefault();
  popup_prompt_factory->ResetCounts();
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_attr_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  // Muted version of above
  ResetAutoplayToDefault();
  popup_prompt_factory->ResetCounts();
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_method_muted_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  ResetAutoplayToDefault();
  popup_prompt_factory->ResetCounts();
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_attr_muted_url());
  EXPECT_TRUE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);
}

// Allow autoplay
IN_PROC_BROWSER_TEST_F(AutoplayPermissionContextBrowserTest, AllowAutoplay) {
  std::string result;
  AllowAutoplay();
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_method_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(autoplay_attr_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  // Muted version of above
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_method_muted_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(autoplay_attr_muted_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

}

// Block autoplay
IN_PROC_BROWSER_TEST_F(AutoplayPermissionContextBrowserTest, BlockAutoplay) {
  std::string result;
  BlockAutoplay();
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_method_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(autoplay_attr_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  // Muted version of above
  result.clear();

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(autoplay_method_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(autoplay_attr_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_NE(result, kVideoPlaying);
}

// Default allow autoplay on file urls
IN_PROC_BROWSER_TEST_F(AutoplayPermissionContextBrowserTest, FileAutoplay) {
  std::string result;
  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());
  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());

  NavigateToURLUntilLoadStop(file_autoplay_method_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);

  result.clear();

  NavigateToURLUntilLoadStop(file_autoplay_attr_url());
  EXPECT_FALSE(popup_prompt_factory->is_visible());
  EXPECT_FALSE(popup_prompt_factory->RequestTypeSeen(
              PermissionRequestType::PERMISSION_AUTOPLAY));
  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  EXPECT_TRUE(ExecuteScriptAndExtractString(contents(),
      kVideoPlayingDetect, &result));
  EXPECT_EQ(result, kVideoPlaying);
}
