/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/scoped_observation.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/request_otr/browser/request_otr_component_installer.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "brave/components/request_otr/common/features.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history/core/browser/history_service.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

using ::testing::_;

namespace {

const char kTestDataDirectory[] = "request-otr-data";

class TestObserver : public infobars::InfoBarManager::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;
  MOCK_METHOD(void, OnInfoBarAdded, (infobars::InfoBar * infobar), (override));
};

}  // namespace

using request_otr::features::kBraveRequestOTR;

namespace request_otr {

class RequestOTRComponentInstallerPolicyWaiter
    : public RequestOTRComponentInstallerPolicy::Observer {
 public:
  explicit RequestOTRComponentInstallerPolicyWaiter(
      RequestOTRComponentInstallerPolicy* component_installer)
      : component_installer_(component_installer), scoped_observer_(this) {
    scoped_observer_.Observe(component_installer_);
  }
  RequestOTRComponentInstallerPolicyWaiter(
      const RequestOTRComponentInstallerPolicyWaiter&) = delete;
  RequestOTRComponentInstallerPolicyWaiter& operator=(
      const RequestOTRComponentInstallerPolicyWaiter&) = delete;
  ~RequestOTRComponentInstallerPolicyWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  // RequestOTRComponentInstallerPolicy::Observer:
  void OnRulesReady(const std::string& json_content) override {
    run_loop_.QuitWhenIdle();
  }

  RequestOTRComponentInstallerPolicy* const component_installer_;
  base::RunLoop run_loop_;
  base::ScopedObservation<RequestOTRComponentInstallerPolicy,
                          RequestOTRComponentInstallerPolicy::Observer>
      scoped_observer_{this};
};

class RequestOTRBrowserTestBase : public BaseLocalDataFilesBrowserTest {
 public:
  // BaseLocalDataFilesBrowserTest overrides
  const char* test_data_directory() override { return kTestDataDirectory; }
  const char* embedded_test_server_directory() override { return ""; }
  LocalDataFilesObserver* service() override {
    return g_brave_browser_process->request_otr_component_installer();
  }

  void WaitForService() override {
    // Wait for request-otr component installer to load and parse its
    // configuration file.
    request_otr::RequestOTRComponentInstallerPolicy* component_installer =
        g_brave_browser_process->request_otr_component_installer();
    RequestOTRComponentInstallerPolicyWaiter(component_installer).Wait();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void SetRequestOTRPref(RequestOTRService::RequestOTRActionOption value) {
    browser()->profile()->GetPrefs()->SetInteger(prefs::kRequestOTRActionOption,
                                                 static_cast<int>(value));
  }

  bool IsShowingInterstitial() {
    return chrome_browser_interstitials::IsShowingInterstitial(web_contents());
  }

  void NavigateTo(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    ASSERT_TRUE(WaitForRenderFrameReady(frame));
  }

  void Click(const std::string& id) {
    content::RenderFrameHost* frame = web_contents()->GetPrimaryMainFrame();
    frame->ExecuteJavaScriptForTests(
        base::ASCIIToUTF16("document.getElementById('" + id + "').click();\n"),
        base::NullCallback());
  }

  void ClickAndWaitForNavigation(const std::string& id) {
    content::TestNavigationObserver observer(web_contents());
    Click(id);
    observer.WaitForNavigationFinished();
  }

  int GetHistoryCount() {
    history::HistoryService* history_service =
        HistoryServiceFactory::GetForProfile(
            browser()->profile(), ServiceAccessType::IMPLICIT_ACCESS);
    CHECK(history_service);
    int history_count = 0;
    base::RunLoop loop;
    base::CancelableTaskTracker task_tracker;
    history_service->GetHistoryCount(
        /*begin_time=*/base::Time::UnixEpoch(),
        /*end_time=*/base::Time::Now(),
        base::BindLambdaForTesting([&](history::HistoryCountResult result) {
          ASSERT_TRUE(result.success);
          history_count = result.count;
          loop.Quit();
        }),
        &task_tracker);
    loop.Run();
    return history_count;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

class RequestOTRBrowserTest : public RequestOTRBrowserTestBase {
 public:
  RequestOTRBrowserTest() {
    feature_list_.InitWithFeatures(
        {kBraveRequestOTR, net::features::kBraveFirstPartyEphemeralStorage},
        {});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, ShowInterstitialNever) {
  ASSERT_TRUE(InstallMockExtension());

  // If request-otr pref is 'never', should not show interstitial even though
  // this URL is included in config file.
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kNever);
  GURL url = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, ShowInterstitialAsk) {
  ASSERT_TRUE(InstallMockExtension());

  // If request-otr pref is 'ask', should show interstitial because
  // this URL is included in config file.
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);
  GURL url = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, ShowInterstitialAlways) {
  ASSERT_TRUE(InstallMockExtension());

  // If request-otr pref is 'always', should not show interstitial but should
  // show infobar indicating to the user that they are navigating in
  // off-the-record mode.
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  TestObserver observer;
  // Set up expectation that an infobar will appear later.
  EXPECT_CALL(observer, OnInfoBarAdded(_)).Times(1);
  infobar_manager->AddObserver(&observer);

  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);
  GURL url = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());
  // Request-OTR infobar should now have been shown, and our observer should
  // have been called once.

  infobar_manager->RemoveObserver(&observer);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, ShowInterstitialAndProceedOTR) {
  ASSERT_TRUE(InstallMockExtension());

  // If request-otr pref is 'ask', should show interstitial because
  // this URL is included in config file. If user clicks 'Proceed
  // Off-The-Record' then we should navigate to the originally requested page
  // and show an infobar indicating to the user that they are navigating in
  // off-the-record mode.
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  TestObserver observer;
  // Set up expectation that an infobar will appear later.
  EXPECT_CALL(observer, OnInfoBarAdded(_)).Times(1);
  infobar_manager->AddObserver(&observer);

  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);
  GURL url = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on "Proceed Off-The-Record" button. This should navigate to
  // the originally requested page in off-the-record mode.
  ClickAndWaitForNavigation("primary-button");
  ASSERT_FALSE(IsShowingInterstitial());
  // Request-OTR infobar should now have been shown, and our observer should
  // have been called once.

  infobar_manager->RemoveObserver(&observer);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       ShowInterstitialAndProceedNormally) {
  ASSERT_TRUE(InstallMockExtension());

  // If request-otr pref is 'ask', we should show an interstitial because this
  // URL is included in config file. If user clicks 'Proceed Normally' then we
  // should navigate to the originally requested page without going into
  // off-the-record mode.
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  TestObserver observer;
  // Set up expectation that an infobar will NOT appear later.
  EXPECT_CALL(observer, OnInfoBarAdded(_)).Times(0);
  infobar_manager->AddObserver(&observer);

  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);
  GURL url = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // Simulate click on 'Proceed Normally' button. This should navigate to
  // the originally requested page in off-the-record mode.
  ClickAndWaitForNavigation("back-button");
  ASSERT_FALSE(IsShowingInterstitial());
  // Request-OTR infobar should never appear, because the user requested to
  // proceed normally, so we should not be in off-the-record mode.

  infobar_manager->RemoveObserver(&observer);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, IncludeExclude) {
  ASSERT_TRUE(InstallMockExtension());
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);
  GURL url1 = embedded_test_server()->GetURL("www.b.com", "/simple.html");
  NavigateTo(url1);
  ASSERT_TRUE(IsShowingInterstitial());
  GURL url2 =
      embedded_test_server()->GetURL("notsensitive.b.com", "/simple.html");
  NavigateTo(url2);
  ASSERT_FALSE(IsShowingInterstitial());
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, HistoryAfterStandardNavigation) {
  ASSERT_EQ(GetHistoryCount(), 0);
  NavigateTo(
      embedded_test_server()->GetURL("notsensitive.b.com", "/simple.html"));
  ASSERT_EQ(GetHistoryCount(), 1);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, HistoryAfterOTRNavigation) {
  ASSERT_TRUE(InstallMockExtension());

  ASSERT_EQ(GetHistoryCount(), 0);
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);
  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_EQ(GetHistoryCount(), 0);
}

class RequestOTRDisabledBrowserTest : public RequestOTRBrowserTestBase {
 public:
  RequestOTRDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(kBraveRequestOTR);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

class RequestOTR1PESBrowserTest : public RequestOTRBrowserTest {
 public:
  RequestOTR1PESBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUp() override {
    // We still need this so InstallMockExtension() can find its files.
    brave::RegisterPathProvider();

    // Reuse upstream test files in content.
    content::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(content::DIR_TEST_DATA, &test_data_dir);

    // We need an HTTPS server to test service workers.
    content::SetupCrossSiteRedirector(&https_server_);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());

    // Bypass BaseLocalDataFilesBrowserTest::SetUp() because we've handled
    // everything already.
    ExtensionBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    RequestOTRBrowserTestBase::SetUpOnMainThread();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(RequestOTR1PESBrowserTest, ServiceWorkerUnavailable) {
  ASSERT_TRUE(InstallMockExtension());

  // Always use request-otr for sensitive sites (skipping interstitial).
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);

  // Sensitive site in request-otr mode should not allow service workers.
  NavigateTo(https_server_.GetURL("sensitive.a.com",
                                  "/workers/service_worker_setup.html"));
  ASSERT_FALSE(content::ExecJs(web_contents(), "setup();"));
}

IN_PROC_BROWSER_TEST_F(RequestOTR1PESBrowserTest, ServiceWorkerAvailable) {
  ASSERT_TRUE(InstallMockExtension());

  // Never use request-otr mode for sensitive sites.
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kNever);

  // Since we are not in request-otr mode, this site should allow service
  // workers.
  NavigateTo(https_server_.GetURL("sensitive.a.com",
                                  "/workers/service_worker_setup.html"));
  ASSERT_TRUE(content::ExecJs(web_contents(), "setup();"));
}

}  // namespace request_otr
