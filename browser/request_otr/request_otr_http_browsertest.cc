/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "brave/components/request_otr/common/features.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_host_resolver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#else
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

namespace {

const char kTestDataDirectory[] = "request-otr-data";
const char kRequestOTRResponseHeader[] = "Request-OTR";

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

namespace request_otr {

// Define a subclass that sets up a special HTTP server that responds with
// a custom header to trigger an OTR tab.
class RequestOTRHTTPBrowserTest : public PlatformBrowserTest {
 public:
  RequestOTRHTTPBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {request_otr::features::kBraveRequestOTRTab,
         net::features::kBraveFirstPartyEphemeralStorage},
        {});
  }
  void SetUp() override {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kTestDataDirectory);
    content::SetupCrossSiteRedirector(embedded_test_server());
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&RespondWithCustomHeader));
    ASSERT_TRUE(embedded_test_server()->Start());
    PlatformBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    PlatformBrowserTest::SetUpOnMainThread();
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }

  bool NavigateToHostAndPath(const std::string& hostname,
                             const std::string& file_path) {
    return content::NavigateToURL(
        GetActiveWebContents(),
        embedded_test_server()->GetURL(hostname, file_path));
  }

  bool IsShowingInterstitial() {
    return chrome_browser_interstitials::IsShowingInterstitial(
        GetActiveWebContents());
  }

  void SetRequestOTRPref(RequestOTRService::RequestOTRActionOption value) {
    GetProfile()->GetPrefs()->SetInteger(kRequestOTRActionOption,
                                         static_cast<int>(value));
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(RequestOTRHTTPBrowserTest,
                       CustomHeaderShowsInterstitial) {
  SetRequestOTRPref(RequestOTRService::RequestOTRActionOption::kAsk);

  // No Request-OTR header -> do not show interstitial
  NavigateToHostAndPath("z.com", "/simple.html");
  ASSERT_FALSE(IsShowingInterstitial());

  // 'Request-OTR: 1' header -> show interstitial
  NavigateToHostAndPath("z.com",
                        "/simple.html?test=include-response-header-with-1");
  ASSERT_TRUE(IsShowingInterstitial());

  // 'Request-OTR: 0' header -> do not show interstitial
  NavigateToHostAndPath("z.com",
                        "/simple.html?test=include-response-header-with-0");
  ASSERT_FALSE(IsShowingInterstitial());
}

}  // namespace request_otr
