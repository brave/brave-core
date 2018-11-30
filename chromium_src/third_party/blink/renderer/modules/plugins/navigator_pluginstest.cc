/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_test_utils.h"

const char kNavigatorPluginsTest[] = "/navigatorplugins.html";
const char kNavigatorMimeTypesTest[] = "/navigatormimetypes.html";
const char kNavigatorMimeTypesTestWithFlash[] = "/navigatormimetypeswithflash.html";

const char kFlashPluginExists[] =
  "domAutomationController.send(Array.from(navigator.plugins).filter("
  "  x => Array.from(x).some("
  "    y => y.type === 'application/x-shockwave-flash')).length)";

class NavigatorPluginsTest : public InProcessBrowserTest {
  public:
    void SetUpOnMainThread() override {
      InProcessBrowserTest::SetUpOnMainThread();

      content_client_.reset(new ChromeContentClient);
      content::SetContentClient(content_client_.get());
      browser_content_client_.reset(new BraveContentBrowserClient());
      content::SetBrowserClientForTesting(browser_content_client_.get());
      content::SetupCrossSiteRedirector(embedded_test_server());

      brave::RegisterPathProvider();
      base::FilePath test_data_dir;
      base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
      embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

      ASSERT_TRUE(embedded_test_server()->Start());
    }

    HostContentSettingsMap * content_settings() {
      return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    }

    void AllowFlash(GURL url) {
      content_settings()->SetContentSettingCustomScope(
          ContentSettingsPattern::FromString(url.spec()),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          std::string(),
          CONTENT_SETTING_ALLOW);
    }

    void TearDown() override {
      browser_content_client_.reset();
      content_client_.reset();
    }

  private:
    std::unique_ptr<ChromeContentClient> content_client_;
    std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
    base::ScopedTempDir temp_user_data_dir_;
};

IN_PROC_BROWSER_TEST_F(NavigatorPluginsTest, ConstPluginsWithoutFlash) {
  GURL url = embedded_test_server()->GetURL(kNavigatorPluginsTest);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool constantPlugins;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(constantPlugins())",
      &constantPlugins));
  EXPECT_TRUE(constantPlugins);
}

IN_PROC_BROWSER_TEST_F(NavigatorPluginsTest, ConstMimeTypesWithoutFlash) {
  GURL url = embedded_test_server()->GetURL(kNavigatorMimeTypesTest);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool constantMimeTypes;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(constantMimeTypes())",
      &constantMimeTypes));
  EXPECT_TRUE(constantMimeTypes);
}


IN_PROC_BROWSER_TEST_F(NavigatorPluginsTest, ConstPluginsAndMimeTypesWithFlash) {
  int len;
  GURL url = embedded_test_server()->GetURL(kNavigatorMimeTypesTestWithFlash);
  AllowFlash(url);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  ASSERT_TRUE(ExecuteScriptAndExtractInt(contents, 
    kFlashPluginExists, &len));
  ASSERT_LE(len, 1);

  // If len == 0, flash is not installed. Skip this test
  if (len == 0) {
    return;
  }

  bool constantMimeTypes;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(constantMimeTypes())",
      &constantMimeTypes));
  EXPECT_TRUE(constantMimeTypes);
}