/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/reload_type.h"
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

  ~SpeedReaderBrowserTest() override = default;

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

  PageActionIconView* GetReaderButton() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->toolbar_button_provider()
        ->GetPageActionIconView(PageActionIconType::kReaderMode);
  }

  void ClickReaderButton() {
    browser()->command_controller()->ExecuteCommand(
        IDC_SPEEDREADER_ICON_ONCLICK);
    content::WaitForLoadStop(ActiveWebContents());
  }

  void ToggleSpeedreader() { speedreader_service()->ToggleSpeedreader(); }

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

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, SmokeTest) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  const std::string kGetStyleLength =
      "document.getElementById('brave_speedreader_style').innerHTML.length";

  const std::string kGetContentLength = "document.body.innerHTML.length";

  const auto eval_js = [&](const std::string& script) {
    int out;
    EXPECT_TRUE(content::ExecuteScriptAndExtractInt(
        ActiveWebContents(),
        "window.domAutomationController.send(" + script + ")", &out));
    return out;
  };

  // Check that the document became much smaller and that non-empty speedreader
  // style is injected.
  EXPECT_LT(0, eval_js(kGetStyleLength));
  EXPECT_GT(17750 + 1, eval_js(kGetContentLength));

  // Check that disabled speedreader doesn't affect the page.
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_LT(106000, eval_js(kGetContentLength));
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, P3ATest) {
  base::HistogramTester tester;

  // SpeedReader never enabled
  EXPECT_FALSE(speedreader_service()->IsEnabled());
  tester.ExpectBucketCount(kSpeedreaderEnabledUMAHistogramName, 0, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 0, 1);

  // SpeedReader recently enabled, toggled once
  ToggleSpeedreader();
  tester.ExpectBucketCount(kSpeedreaderEnabledUMAHistogramName, 2, 2);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 1, 1);
  tester.ExpectBucketCount(kSpeedreaderToggleUMAHistogramName, 2, 0);
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ClickingOnReaderButton) {
  EXPECT_FALSE(speedreader_service()->IsEnabled());

  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  EXPECT_EQ(speedreader::DistillState::kPageProbablyReadable,
            tab_helper()->PageDistillState());
  ClickReaderButton();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kReaderMode,
            tab_helper()->PageDistillState());
  EXPECT_TRUE(GetReaderButton()->GetVisible());

  ClickReaderButton();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kPageProbablyReadable,
            tab_helper()->PageDistillState());

  EXPECT_FALSE(speedreader_service()->IsEnabled());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, EnableDisableSpeedreader) {
  EXPECT_FALSE(speedreader_service()->IsEnabled());
  NavigateToPageSynchronously(kTestPageReadable);

  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kPageProbablyReadable,
            tab_helper()->PageDistillState());
  ToggleSpeedreader();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderOnDisabledPage,
            tab_helper()->PageDistillState());
  DisableSpeedreader();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kPageProbablyReadable,
            tab_helper()->PageDistillState());

  ClickReaderButton();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kReaderMode,
            tab_helper()->PageDistillState());
  ToggleSpeedreader();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderMode,
            tab_helper()->PageDistillState());
  DisableSpeedreader();
  EXPECT_TRUE(GetReaderButton()->GetVisible());
  EXPECT_EQ(speedreader::DistillState::kReaderMode,
            tab_helper()->PageDistillState());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, TogglingSiteSpeedreader) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);

  for (int i = 0; i < 2; ++i) {
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
    EXPECT_EQ(speedreader::DistillState::kSpeedreaderMode,
              tab_helper()->PageDistillState());
    EXPECT_TRUE(GetReaderButton()->GetVisible());

    tab_helper()->MaybeToggleEnabledForSite(false);
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
    EXPECT_EQ(speedreader::DistillState::kSpeedreaderOnDisabledPage,
              tab_helper()->PageDistillState());
    EXPECT_TRUE(GetReaderButton()->GetVisible());

    tab_helper()->MaybeToggleEnabledForSite(true);
    EXPECT_TRUE(WaitForLoadStop(ActiveWebContents()));
  }
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ReloadContent) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents_1 = ActiveWebContents();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* contents_2 = ActiveWebContents();

  auto* tab_helper_1 =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents_1);
  auto* tab_helper_2 =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents_2);

  EXPECT_EQ(speedreader::DistillState::kSpeedreaderMode,
            tab_helper_1->PageDistillState());
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderMode,
            tab_helper_2->PageDistillState());

  tab_helper_1->MaybeToggleEnabledForSite(false);
  content::WaitForLoadStop(contents_1);
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderOnDisabledPage,
            tab_helper_1->PageDistillState());
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderMode,
            tab_helper_2->PageDistillState());

  contents_2->GetController().Reload(content::ReloadType::NORMAL, false);
  content::WaitForLoadStop(contents_2);

  EXPECT_EQ(speedreader::DistillState::kSpeedreaderOnDisabledPage,
            tab_helper_1->PageDistillState());
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderOnDisabledPage,
            tab_helper_2->PageDistillState());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ShowOriginalPage) {
  const std::u16string title = u"\u0022\"script shouldn't fail\"\u0022";
  speedreader::test::SetShowOriginalLinkTitle(&title);

  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);
  auto* web_contents = ActiveWebContents();

  constexpr const char kCheckNoApiInMainWorld[] =
      R"js(
        document.speedreader === undefined
      )js";
  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoApiInMainWorld,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS)
                  .ExtractBool());

  constexpr const char kClickLinkAndGetTitle[] =
      R"js(
    (function() {
      // element id is hardcoded in extractor.rs
      const link =
        document.getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e');
      link.click();
      return link.text
    })();
  )js";

  EXPECT_EQ(base::UTF16ToUTF8(title),
            content::EvalJs(web_contents, kClickLinkAndGetTitle,
                            content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                            speedreader::kIsolatedWorldId)
                .ExtractString());
  content::WaitForLoadStop(web_contents);
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderOnDisabledPage,
            tab_helper->PageDistillState());
  EXPECT_TRUE(tab_helper->IsEnabledForSite());

  // Click on speedreader button
  ClickReaderButton();
  content::WaitForLoadStop(web_contents);
  EXPECT_EQ(speedreader::DistillState::kSpeedreaderMode,
            tab_helper->PageDistillState());

  speedreader::test::SetShowOriginalLinkTitle(nullptr);
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, ShowOriginalPageOnUnreadable) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageSimple);
  auto* web_contents = ActiveWebContents();

  constexpr const char kCheckNoElement[] =
      R"js(
        document.getElementById('c93e2206-2f31-4ddc-9828-2bb8e8ed940e') == null
      )js";

  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoElement,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              speedreader::kIsolatedWorldId)
                  .ExtractBool());

  constexpr const char kCheckNoApi[] =
      R"js(
        document.speedreader === undefined
      )js";

  EXPECT_TRUE(content::EvalJs(web_contents, kCheckNoApi,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              speedreader::kIsolatedWorldId)
                  .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SpeedReaderBrowserTest, SetTheme) {
  ToggleSpeedreader();
  NavigateToPageSynchronously(kTestPageReadable);

  constexpr const char kGetTheme[] =
      R"js(
        document.documentElement.getAttribute('data-theme')
      )js";

  EXPECT_EQ(speedreader::Theme::kNone, speedreader_service()->GetTheme());

  EXPECT_EQ(nullptr, content::EvalJs(ActiveWebContents(), kGetTheme,
                                     content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                     speedreader::kIsolatedWorldId));
  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(ActiveWebContents());
  tab_helper->SetTheme(speedreader::Theme::kDark);

  EXPECT_EQ("dark", content::EvalJs(ActiveWebContents(), kGetTheme,
                                    content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                    speedreader::kIsolatedWorldId)
                        .ExtractString());
  EXPECT_EQ(speedreader::Theme::kDark, speedreader_service()->GetTheme());

  // New page
  NavigateToPageSynchronously(kTestPageReadable);
  EXPECT_EQ("dark", content::EvalJs(ActiveWebContents(), kGetTheme,
                                    content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                    speedreader::kIsolatedWorldId)
                        .ExtractString());
}
