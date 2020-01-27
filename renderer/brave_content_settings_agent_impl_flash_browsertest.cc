/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/permission_bubble/mock_permission_prompt_factory.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/ppapi_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/dns/mock_host_resolver.h"

const char kFlashPluginExists[] =
  "domAutomationController.send(Array.from(navigator.plugins).filter("
  "  x => Array.from(x).some("
  "    y => y.type === 'application/x-shockwave-flash')).length)";

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

class BraveContentSettingsAgentImplFlashBrowserTest
    : public InProcessBrowserTest {
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

    url_ = embedded_test_server()->GetURL("a.com", "/flash.html");
    top_level_page_pattern_ =
        ContentSettingsPattern::FromString("http://a.com/*");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    ASSERT_TRUE(ppapi::RegisterFlashTestPlugin(command_line));
    // These tests are for the permission prompt to add and remove Flash from
    // navigator.plugins. We disable Plugin Power Saver, because its plugin
    // throttling make it harder to test if Flash was succcessfully enabled.
    command_line->AppendSwitchASCII(
        switches::kOverridePluginPowerSaverForTesting, "never");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  const GURL& url() { return url_; }

  const ContentSettingsPattern& top_level_page_pattern() {
    return top_level_page_pattern_;
  }

  const ContentSettingsPattern& empty_pattern() { return empty_pattern_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void UnblockFlash() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern_, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::PLUGINS, std::string(),
        CONTENT_SETTING_DETECT_IMPORTANT_CONTENT);
  }

  void AllowFlash() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern_, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::PLUGINS, std::string(), CONTENT_SETTING_ALLOW);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

 private:
  GURL url_;
  ContentSettingsPattern top_level_page_pattern_;
  ContentSettingsPattern empty_pattern_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;

  base::ScopedTempDir temp_user_data_dir_;
};

// Flash is blocked by default
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplFlashBrowserTest,
    BlockFlashByDefault) {
  NavigateToURLUntilLoadStop(url());
  int len;
  ASSERT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kFlashPluginExists, &len));
  ASSERT_EQ(len, 0);
}

// Flash is unblocked and click to play eventually allows
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplFlashBrowserTest,
    UnblockFlash) {
  UnblockFlash();
  NavigateToURLUntilLoadStop(url());
  int len;
  ASSERT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kFlashPluginExists, &len));
  ASSERT_EQ(len, 0);

  PermissionRequestManager* manager = PermissionRequestManager::FromWebContents(
      contents());

  auto popup_prompt_factory =
      std::make_unique<MockPermissionPromptFactory>(manager);

  EXPECT_EQ(0, popup_prompt_factory->TotalRequestCount());
  popup_prompt_factory->set_response_type(PermissionRequestManager::ACCEPT_ALL);

  PageReloadWaiter reload_waiter(contents());

  bool value;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(), "triggerPrompt();",
      &value));
  EXPECT_TRUE(value);
  EXPECT_TRUE(reload_waiter.Wait());

  EXPECT_EQ(1, popup_prompt_factory->TotalRequestCount());

  // Shut down the popup window tab, as the normal test teardown assumes there
  // is only one test tab.
  popup_prompt_factory.reset();

  ASSERT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kFlashPluginExists, &len));
  ASSERT_GT(len, 0);
}

// Flash is explicitly allowed
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplFlashBrowserTest,
    AllowFlashExplicitAllows) {
  AllowFlash();
  NavigateToURLUntilLoadStop(url());
  int len;
  ASSERT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kFlashPluginExists, &len));
  ASSERT_GT(len, 0);
}

