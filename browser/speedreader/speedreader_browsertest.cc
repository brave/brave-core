/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/path_service.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/components/speedreader/features.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

const char kTestHost[] = "a.test";
const char kTestPageSimple[] = "/simple.html";
const char kTestPageReadable[] = "/articles/guardian.html";

constexpr char kSpeedreaderToggleUMAHistogramName[] =
    "Brave.SpeedReader.ToggleCount";

constexpr char kSpeedreaderEnabledUMAHistogramName[] =
    "Brave.SpeedReader.Enabled";

class SpeedReaderBrowserTest : public InProcessBrowserTest {
 public:
  SpeedReaderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(speedreader::kSpeedreaderFeature);
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    EXPECT_TRUE(https_server_.Start());
  }

  SpeedReaderBrowserTest(const SpeedReaderBrowserTest&) = delete;
  SpeedReaderBrowserTest& operator=(const SpeedReaderBrowserTest&) = delete;

  ~SpeedReaderBrowserTest() override {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void TearDownOnMainThread() override { DisableSpeedreader(); }

  content::WebContents* ActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  speedreader::SpeedreaderTabHelper* tab_helper() {
    return speedreader::SpeedreaderTabHelper::FromWebContents(
        ActiveWebContents());
  }

  speedreader::SpeedreaderService* speedreader_service() {
    return speedreader::SpeedreaderServiceFactory::GetForProfile(
        browser()->profile());
  }

  void ToggleSpeedreader() {
    speedreader_service()->ToggleSpeedreader();
    ActiveWebContents()->GetController().Reload(content::ReloadType::NORMAL,
                                                false);
  }

  void DisableSpeedreader() {
    speedreader_service()->DisableSpeedreaderForTest();
  }

  void GoBack(Browser* browser) {
    content::TestNavigationObserver observer(ActiveWebContents());
    chrome::GoBack(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

  void NavigateToPageSynchronously(
      const char* path,
      WindowOpenDisposition disposition =
          WindowOpenDisposition::NEW_FOREGROUND_TAB) {
    const GURL url = https_server_.GetURL(kTestHost, path);
    ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, disposition,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, RestoreSpeedreaderPage) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));

  Profile* profile = browser()->profile();

  ScopedKeepAlive test_keep_alive(KeepAliveOrigin::PANEL_VIEW,
                                  KeepAliveRestartOption::DISABLED);
  ScopedProfileKeepAlive test_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kBrowserWindow);
  CloseBrowserSynchronously(browser());

  EXPECT_EQ(0u, BrowserList::GetInstance()->size());
  chrome::OpenWindowWithRestoredTabs(profile);
  EXPECT_EQ(1u, BrowserList::GetInstance()->size());
  SelectFirstBrowser();
  EXPECT_TRUE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, NavigationNostickTest) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageSimple);
  EXPECT_FALSE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));
  NavigateToPageSynchronously(kTestPageReadable,
                              WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));

  // Ensure distill state doesn't stick when we back-navigate from a readable
  // page to a non-readable one.
  GoBack(browser());
  EXPECT_FALSE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DisableSiteWorks) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));
  tab_helper()->MaybeToggleEnabledForSite(false);
  EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
  EXPECT_FALSE(
      speedreader::PageStateIsDistilled(tab_helper()->PageDistillState()));
}

// disabled in https://github.com/brave/brave-browser/issues/11328
IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, DISABLED_SmokeTest) {
  ToggleSpeedreader();
  const GURL url = https_server_.GetURL(kTestHost, kTestPageReadable);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents = ActiveWebContents();
  content::RenderFrameHost* rfh = contents->GetMainFrame();

  const char kGetStyleLength[] =
      "document.getElementById(\"brave_speedreader_style\").innerHTML.length";

  const char kGetContentLength[] = "document.body.innerHTML.length";

  // Check that the document became much smaller and that non-empty speedreader
  // style is injected.
  EXPECT_LT(0, content::EvalJs(rfh, kGetStyleLength));
  EXPECT_GT(17750 + 1, content::EvalJs(rfh, kGetContentLength));

  // Check that disabled speedreader doesn't affect the page.
  ToggleSpeedreader();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  rfh = contents->GetMainFrame();
  EXPECT_LT(106000, content::EvalJs(rfh, kGetContentLength));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, P3ATest) {
  base::HistogramTester tester;

  // SpeedReader never enabled
  EXPECT_FALSE(speedreader_service()->IsEnabled());
  tester.ExpectBucketCount(kSpeedreaderEnabledUMAHistogramName, 0, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 0, 1);

  // SpeedReader recently enabled, toggled once
  ToggleSpeedreader();
  tester.ExpectBucketCount(kSpeedreaderEnabledUMAHistogramName, 2, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 1, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 2, 0);
}
