/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
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
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/features.h"
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
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

using ::testing::_;

namespace {

constexpr char kTestDataDirectory[] = "request-otr-data";
constexpr char kRequestOTRResponseHeader[] = "Request-OTR";

class TestObserver : public infobars::InfoBarManager::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;
  MOCK_METHOD(void, OnInfoBarAdded, (infobars::InfoBar * infobar), (override));
};

std::unique_ptr<net::test_server::HttpResponse> RespondWithCustomHeader(
    const net::test_server::HttpRequest& request) {
  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/plain");
  http_response->set_content("Well OK I guess");
  if (request.relative_url.find("include-response-header-with-1") !=
      std::string::npos) {
    http_response->AddCustomHeader(kRequestOTRResponseHeader, "1");
  } else if (request.relative_url.find("include-response-header-with-0") !=
             std::string::npos) {
    http_response->AddCustomHeader(kRequestOTRResponseHeader, "0");
  }
  return http_response;
}

}  // namespace

using request_otr::features::kBraveRequestOTRTab;

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

  raw_ptr<RequestOTRComponentInstallerPolicy> const component_installer_;
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
    browser()->profile()->GetPrefs()->SetInteger(kRequestOTRActionOption,
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
        base::NullCallback(), content::ISOLATED_WORLD_ID_GLOBAL);
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
        {kBraveRequestOTRTab, net::features::kBraveFirstPartyEphemeralStorage},
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

// Check that a URL affected by both include and exclude rules is properly
// excluded.
IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       URLThatIsIncludedAndExcludedIsExcluded) {
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

// Check that URLs ending with a '.' are properly included or excluded.
IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest, URLThatEndsWithADot) {
  ASSERT_TRUE(InstallMockExtension());
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);
  GURL url1 = embedded_test_server()->GetURL("www.b.com.", "/simple.html");
  NavigateTo(url1);
  ASSERT_TRUE(IsShowingInterstitial());
  GURL url2 =
      embedded_test_server()->GetURL("notsensitive.b.com.", "/simple.html");
  NavigateTo(url2);
  ASSERT_FALSE(IsShowingInterstitial());
}

// Check that URLs are added to history after navigation. (This is a sanity
// check.)
IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       HistoryRecordedAfterNonOTRNavigation) {
  ASSERT_EQ(GetHistoryCount(), 0);
  NavigateTo(
      embedded_test_server()->GetURL("notsensitive.b.com", "/simple.html"));
  ASSERT_EQ(GetHistoryCount(), 1);
}

// Now check that URLs are not added to history after navigation in
// Request-OTR-tab mode.
IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       HistoryNotRecordedAfterOTRNavigation) {
  ASSERT_TRUE(InstallMockExtension());

  ASSERT_EQ(GetHistoryCount(), 0);
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);
  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_EQ(GetHistoryCount(), 0);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       WindowOpenAfterStandardNavigationCrossOrigin) {
  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_TRUE(content::ExecJs(
      web_contents(), "window.open('notsensitive.b.com/simple.html');"));
  ASSERT_NE(content::EvalJs(web_contents(), "window.opener"), nullptr);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       WindowOpenAfterStandardNavigationSameOrigin) {
  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_TRUE(
      content::ExecJs(web_contents(), "window.open('a.com/simple.html');"));
  ASSERT_NE(content::EvalJs(web_contents(), "window.opener"), nullptr);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       WindowOpenAfterOTRNavigationCrossOrigin) {
  ASSERT_TRUE(InstallMockExtension());

  // Always use request-otr for sensitive sites (skipping interstitial).
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);

  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_TRUE(content::ExecJs(
      web_contents(), "window.open('notsensitive.b.com/simple.html');"));
  ASSERT_EQ(content::EvalJs(web_contents(), "window.opener"), nullptr);
}

IN_PROC_BROWSER_TEST_F(RequestOTRBrowserTest,
                       WindowOpenAfterOTRNavigationSameOrigin) {
  ASSERT_TRUE(InstallMockExtension());

  // Always use request-otr for sensitive sites (skipping interstitial).
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);

  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_TRUE(
      content::ExecJs(web_contents(), "window.open('a.com/simple.html');"));
  ASSERT_EQ(content::EvalJs(web_contents(), "window.opener"), nullptr);
}

// Define a subclass that disables the feature so we can ensure that nothing
// happens when the feature is disabled through runtime flags.
class RequestOTRDisabledBrowserTest : public RequestOTRBrowserTestBase {
 public:
  RequestOTRDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(kBraveRequestOTRTab);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Ensure that we do not show the Request-OTR-tab interstitial if the runtime
// feature flag is disabled.
IN_PROC_BROWSER_TEST_F(RequestOTRDisabledBrowserTest,
                       DoNotShowInterstitialIfFeatureDisabled) {
  InstallMockExtension();
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);
  GURL url1 = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url1);
  ASSERT_FALSE(IsShowingInterstitial());
}

// Ensure that we do not show the Request-OTR-tab infobar if the runtime
// feature flag is disabled.
IN_PROC_BROWSER_TEST_F(RequestOTRDisabledBrowserTest,
                       DoNotShowInfobarIfFeatureDisabled) {
  InstallMockExtension();
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  TestObserver observer;
  // Set up expectation that an infobar will NOT appear later.
  EXPECT_CALL(observer, OnInfoBarAdded(_)).Times(0);
  infobar_manager->AddObserver(&observer);

  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);
  GURL url = embedded_test_server()->GetURL("sensitive.a.com", "/simple.html");
  NavigateTo(url);

  // Request-OTR infobar should never appear, because the user requested to
  // proceed normally, so we should not be in off-the-record mode.
  infobar_manager->RemoveObserver(&observer);
}

// URLs should be added to history after navigation, even if Request-OTR
// preference is set to 'always' and URL matches a sensitive site from the
// configuration file, because the runtime feature flag is disabled.
IN_PROC_BROWSER_TEST_F(RequestOTRDisabledBrowserTest,
                       HistoryRecordedIfFeatureDisabled) {
  InstallMockExtension();

  ASSERT_EQ(GetHistoryCount(), 0);
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);
  NavigateTo(embedded_test_server()->GetURL("sensitive.a.com", "/simple.html"));
  ASSERT_EQ(GetHistoryCount(), 1);
}

// Define a subclass that sets up an HTTPS server and serves data from a
// different directory in order to reuse service worker scripts from upstream
// tests.
class RequestOTRServiceWorkerBrowserTest : public RequestOTRBrowserTest {
 public:
  RequestOTRServiceWorkerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    base::FilePath test_data_dir;
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(content::DIR_TEST_DATA, &test_data_dir);

    // We need an HTTPS server to test service workers.
    content::SetupCrossSiteRedirector(&https_server_);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());

    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

    // Bypass BaseLocalDataFilesBrowserTest::SetUpOnMainThread() because we've
    // handled everything already.
    ExtensionBrowserTest::SetUpOnMainThread();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(RequestOTRServiceWorkerBrowserTest,
                       ServiceWorkerUnavailable) {
  ASSERT_TRUE(InstallMockExtension());

  // Always use request-otr for sensitive sites (skipping interstitial).
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAlways);

  // Sensitive site in request-otr mode should not allow service workers.
  NavigateTo(https_server_.GetURL("sensitive.a.com",
                                  "/workers/service_worker_setup.html"));
  ASSERT_FALSE(content::ExecJs(web_contents(), "setup();"));
}

IN_PROC_BROWSER_TEST_F(RequestOTRServiceWorkerBrowserTest,
                       ServiceWorkerAvailable) {
  ASSERT_TRUE(InstallMockExtension());

  // Never use request-otr mode for sensitive sites.
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kNever);

  // Since we are not in request-otr mode, this site should allow service
  // workers.
  NavigateTo(https_server_.GetURL("sensitive.a.com",
                                  "/workers/service_worker_setup.html"));
  ASSERT_TRUE(content::ExecJs(web_contents(), "setup();"));
}

// Define a subclass that sets up a special HTTP server that responds with
// a custom header to trigger an OTR tab.
class RequestOTRCustomHeaderBrowserTest : public RequestOTRBrowserTest {
 public:
  void SetUpOnMainThread() override {
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&RespondWithCustomHeader));
    ASSERT_TRUE(embedded_test_server()->Start());
    host_resolver()->AddRule("*", "127.0.0.1");

    // Bypass BaseLocalDataFilesBrowserTest::SetUpOnMainThread() because we've
    // handled everything already.
    ExtensionBrowserTest::SetUpOnMainThread();
  }
};

IN_PROC_BROWSER_TEST_F(RequestOTRCustomHeaderBrowserTest,
                       CustomHeaderShowsInterstitial) {
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);

  // No Request-OTR header -> do not show interstitial
  GURL url = embedded_test_server()->GetURL("z.com", "/simple.html");
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());

  // 'Request-OTR: 1' header -> show interstitial
  url = embedded_test_server()->GetURL(
      "z.com", "/simple.html?test=include-response-header-with-1");
  NavigateTo(url);
  ASSERT_TRUE(IsShowingInterstitial());

  // 'Request-OTR: 0' header -> do not show interstitial
  url = embedded_test_server()->GetURL(
      "z.com", "/simple.html?test=include-response-header-with-0");
  NavigateTo(url);
  ASSERT_FALSE(IsShowingInterstitial());
}

}  // namespace request_otr
