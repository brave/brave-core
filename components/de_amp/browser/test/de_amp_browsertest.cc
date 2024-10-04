/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/stringprintf.h"
#include "brave/components/de_amp/common/features.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/reload_type.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_MAC)
#include "content/public/common/content_features.h"
#endif

namespace {

constexpr char kTestHost[] = "a.test.com";
constexpr char kTestAmpPage[] = "/test_amp_page";
constexpr char kTestRedirectingAmpPage1[] = "/redirecting_amp_page_1";
constexpr char kTestRedirectingAmpPage2[] = "/redirecting_amp_page_2";
constexpr char kTestSimpleNonAmpPage[] = "/simple_page";
constexpr char kTestCanonicalPage[] = "/simple_canonical_page";
constexpr char kTestAmpBodyScaffolding[] =
    R"(
    <html amp>
    <head>
    %s
    </head>
    </html>
    )";
constexpr uint32_t kTestReadBufferSize = 65536;  // bytes
constexpr char kTestAmpCanonicalLink[] = "<link rel='canonical' href='%s'>";

// HELPERS
std::unique_ptr<net::test_server::HttpResponse> BuildHttpResponse(
    const std::string& body,
    net::HttpStatusCode code = net::HttpStatusCode::HTTP_OK,
    const std::map<std::string, std::string>& custom_headers = {},
    const std::string& content_type = "text/html") {
  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(code);
  for (auto& [k, v] : custom_headers) {
    http_response->AddCustomHeader(k, v);
  }

  http_response->set_content_type(content_type);
  http_response->set_content(body);
  return http_response;
}

std::string Location(const std::string& path) {
  return "https://" + (kTestHost + path);
}

std::string Amp(const GURL& custom_url) {
  return base::StringPrintf(
      kTestAmpBodyScaffolding,
      base::StringPrintf(kTestAmpCanonicalLink, custom_url.spec().c_str())
          .c_str());
}

std::string Amp(const std::string& path) {
  return Amp(GURL(Location(path)));
}

std::string Canonical(const std::string& custom_head = "It's canonical") {
  return base::StringPrintf(kTestAmpBodyScaffolding, custom_head.c_str());
}

}  // namespace

class DeAmpBrowserTest : public InProcessBrowserTest {
 public:
  DeAmpBrowserTest() {
    std::vector<base::test::FeatureRef> disabled_features = {};
#if BUILDFLAG(IS_MAC)
    // On Mac, the DeAmpBrowserTest.AmpURLNotStoredInHistory test crashes
    // due to https://crbug.com/1284500: DCHECK in
    // blink::ContentToVisibleTimeReporter::TabWasShown when BFCache is
    // used. To get around the crash, disabling BFCache for these tests
    // until the upstream bug is fixed.
    disabled_features.push_back({features::kBackForwardCache});
#endif  // BUILDFLAG(IS_MAC)
    feature_list_.InitWithFeatures(
        /*enabled_features*/ {de_amp::features::kBraveDeAMP},
        disabled_features);

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
  }

  void SetUp() override {
    EXPECT_TRUE(https_server_->InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    prefs_ = browser()->profile()->GetPrefs();

    content::SetupCrossSiteRedirector(https_server_.get());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP *:443 " + https_server_->host_port_pair().ToString());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void StartServer() { https_server_->StartAcceptingConnections(); }

  void SetRequestHandler(
      const std::string& page_path,
      const std::string& body,
      net::HttpStatusCode code = net::HttpStatusCode::HTTP_OK,
      const std::map<std::string, std::string>& custom_headers = {},
      const std::string& content_type = "text/html") {
    auto handler = [](const std::string& page_path, const std::string& body,
                      net::HttpStatusCode code,
                      const std::map<std::string, std::string>& custom_headers,
                      const std::string& content_type,
                      const net::test_server::HttpRequest& request)
        -> std::unique_ptr<net::test_server::HttpResponse> {
      [&request]() {
        // This should never happen, abort test
        ASSERT_EQ(request.headers.find("X-Brave-De-AMP"),
                  request.headers.end());
      }();

      if (page_path != "*" && request.relative_url != page_path) {
        return nullptr;
      }

      return BuildHttpResponse(body, code, custom_headers, content_type);
    };

    https_server_->RegisterRequestHandler(
        base::BindRepeating(std::move(handler), page_path, body, code,
                            custom_headers, content_type));
  }

  void TogglePref(const bool on) {
    prefs_->SetBoolean(de_amp::kDeAmpPrefEnabled, on);
    web_contents()->GetController().Reload(content::ReloadType::NORMAL, false);
  }

  void NavigateToURLAndWaitForRedirects(const std::string& original_page,
                                        const std::string& landing_page) {
    const auto original_url = GURL(Location(original_page));
    const auto landing_url = GURL(Location(landing_page));

    ui_test_utils::UrlLoadObserver load_complete(landing_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), original_url));
    load_complete.Wait();
    EXPECT_EQ(web_contents()->GetLastCommittedURL(), landing_url);
  }

  void GoBack(Browser* browser) {
    content::TestNavigationObserver observer(web_contents());
    chrome::GoBack(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

  void GoForward(Browser* browser) {
    content::TestNavigationObserver observer(web_contents());
    chrome::GoForward(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  raw_ptr<PrefService> prefs_;
};

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, SimpleDeAmp) {
  TogglePref(true);
  SetRequestHandler(kTestSimpleNonAmpPage, Canonical());
  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  SetRequestHandler(kTestCanonicalPage, Canonical());
  SetRequestHandler(kTestRedirectingAmpPage1, Amp(kTestCanonicalPage),
                    net::HttpStatusCode::HTTP_OK, {}, "text/plain");
  StartServer();

  // Go to any page
  const GURL simple = https_server_->GetURL(kTestHost, kTestSimpleNonAmpPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), simple));
  EXPECT_EQ(web_contents()->GetLastCommittedURL(), simple);

  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestCanonicalPage);

  // Non-HTML page should not be De-AMPed.
  NavigateToURLAndWaitForRedirects(kTestRedirectingAmpPage1,
                                   kTestRedirectingAmpPage1);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalLinkOutsideChunkWithinMax) {
  TogglePref(true);
  // Construct page with large <head>
  const std::string large_string = base::StringPrintf(
      "%s\n%s", std::string(kTestReadBufferSize, 'a').c_str(),
      base::StringPrintf(kTestAmpCanonicalLink,
                         Location(kTestCanonicalPage).c_str())
          .c_str());
  const std::string amp_body_large =
      base::StringPrintf(kTestAmpBodyScaffolding, large_string.c_str());

  SetRequestHandler(kTestCanonicalPage, Canonical());
  SetRequestHandler(kTestAmpPage, amp_body_large);
  StartServer();

  // Now go to an AMP page
  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestCanonicalPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalLinkOutsideChunkOutsideMax) {
  TogglePref(true);
  // Construct page with large <head>
  const std::string large_string = base::StringPrintf(
      "%s\n%s", std::string(3 * kTestReadBufferSize, 'a').c_str(),
      base::StringPrintf(kTestAmpCanonicalLink,
                         Location(kTestCanonicalPage).c_str())
          .c_str());
  const std::string amp_body_large =
      base::StringPrintf(kTestAmpBodyScaffolding, large_string.c_str());

  SetRequestHandler(kTestCanonicalPage, Canonical());
  SetRequestHandler(kTestAmpPage, amp_body_large);
  StartServer();

  // Now go to an AMP page
  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, AmpPagesPointingAtEachOther) {
  TogglePref(true);

  // Make a cycle
  SetRequestHandler(kTestRedirectingAmpPage1, Amp(kTestRedirectingAmpPage2));
  SetRequestHandler(kTestRedirectingAmpPage2, Amp(kTestRedirectingAmpPage1));
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestRedirectingAmpPage1,
                                   kTestRedirectingAmpPage2);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalRedirectsToAmp301) {
  TogglePref(true);
  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  SetRequestHandler(kTestCanonicalPage, Canonical(),
                    net::HttpStatusCode::HTTP_PERMANENT_REDIRECT,
                    {{"Location", Location(kTestAmpPage)}});
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(kTestCanonicalPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalRedirectsToAmp302) {
  TogglePref(true);

  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  SetRequestHandler(kTestCanonicalPage, Canonical(),
                    net::HttpStatusCode::HTTP_FOUND,
                    {{"Location", Location(kTestAmpPage)}});
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(kTestCanonicalPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalJSRedirectsToAmp) {
  TogglePref(true);

  const std::string script = R"()
      <script type="text/javascript">
         window.location.replace(')" +
                             Location(kTestAmpPage) + R"(')</script>)";

  // Load canonical page normally and then navigate to AMP page
  SetRequestHandler(kTestCanonicalPage, Canonical(script));
  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
  NavigateToURLAndWaitForRedirects(kTestCanonicalPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, NonHttpScheme) {
  TogglePref(true);
  SetRequestHandler(kTestAmpPage, Amp(GURL("brave://settings")));
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, CanonicalLinkSameAsAmpPage) {
  TogglePref(true);
  SetRequestHandler(kTestAmpPage, Amp(kTestAmpPage));
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, PrefOff) {
  TogglePref(false);
  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  StartServer();
  // Doesn't get De-AMPed
  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, RestoreTab) {
  TogglePref(true);
  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  StartServer();
  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestCanonicalPage);
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

  EXPECT_EQ(web_contents()->GetLastCommittedURL().path(), kTestCanonicalPage);
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, AmpURLNotStoredInHistory) {
  TogglePref(true);
  SetRequestHandler("/simple", Canonical());
  SetRequestHandler("/amp_1", Amp("/canonical_1"));
  SetRequestHandler("/amp_2", Amp("/canonical_2"));
  SetRequestHandler("/canonical_1", Canonical());
  SetRequestHandler("/canonical_2", Canonical());

  StartServer();

  // Go to any page
  NavigateToURLAndWaitForRedirects("/simple", "/simple");

  // Now go to an AMP page
  NavigateToURLAndWaitForRedirects("/amp_1", "/canonical_1");

  // Go to another AMP page
  NavigateToURLAndWaitForRedirects("/amp_2", "/canonical_2");

  // Going back and forward in history should ignore the non-AMP page
  GoBack(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL().path(), "/canonical_1");
  GoBack(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL().path(), "/simple");

  GoForward(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL().path(), "/canonical_1");
  GoForward(browser());
  EXPECT_EQ(web_contents()->GetLastCommittedURL().path(), "/canonical_2");
}

// Inspired by view-source test:
// chrome/browser/tab_contents/view_source_browsertest.cc
IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, NonDeAmpedPageSameAsOriginal) {
  TogglePref(true);
  SetRequestHandler(kTestSimpleNonAmpPage, Canonical("simple"));
  StartServer();

  NavigateToURLAndWaitForRedirects(kTestSimpleNonAmpPage,
                                   kTestSimpleNonAmpPage);
  content::RenderFrameHost* current_main_frame =
      web_contents()->GetPrimaryMainFrame();
  // Open View Source for page
  content::WebContentsAddedObserver view_source_contents_observer;
  current_main_frame->ViewSource();
  content::WebContents* view_source_contents =
      view_source_contents_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(view_source_contents));

  std::string source_text;
  // Get contents of view-source'd tab
  std::string view_source_extraction_script =
      R"js(
        const nodes = Array.from(document.querySelectorAll(".line-content"))
        nodes.reduce((prev, cur) => prev + cur.innerText + "\n", "")
      )js";
  std::string actual_page_body =
      EvalJs(view_source_contents, view_source_extraction_script)
          .ExtractString();
  EXPECT_THAT(actual_page_body, testing::HasSubstr(Canonical("simple")));
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, AmpPagesChain) {
  TogglePref(true);
  SetRequestHandler("/amp_1", Amp("/amp_2"));
  SetRequestHandler("/amp_2", Amp("/amp_3"));
  SetRequestHandler("/amp_3", Amp("/amp_4"));
  SetRequestHandler("/amp_4", Amp("/canonical"));
  SetRequestHandler("/canonical", Canonical());
  StartServer();

  NavigateToURLAndWaitForRedirects("/amp_1", "/canonical");
}

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTest, AmpPagesCycledChain) {
  TogglePref(true);
  SetRequestHandler("/amp_1", Amp("/amp_2"));
  SetRequestHandler("/amp_2", Amp("/amp_3"));
  SetRequestHandler("/amp_3", Amp("/amp_4"));
  SetRequestHandler("/amp_4", Amp("/amp_2"));
  StartServer();

  NavigateToURLAndWaitForRedirects("/amp_1", "/amp_4");
}

class DeAmpBrowserTestBaseFeatureDisabled : public DeAmpBrowserTest {
 public:
  DeAmpBrowserTestBaseFeatureDisabled() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(de_amp::features::kBraveDeAMP);
  }
};

IN_PROC_BROWSER_TEST_F(DeAmpBrowserTestBaseFeatureDisabled, DoesNotDeAmp) {
  SetRequestHandler(kTestAmpPage, Amp(kTestCanonicalPage));
  StartServer();

  // Doesn't get De-AMPed
  NavigateToURLAndWaitForRedirects(kTestAmpPage, kTestAmpPage);
}
