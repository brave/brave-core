/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/captcha_metrics/captcha_metrics.h"

#include <memory>
#include <string_view>

#include "base/strings/string_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/misc_metrics/features.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_initialize.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/page_load_metrics/browser/page_load_metrics_test_waiter.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "url/gurl.h"

namespace misc_metrics {

namespace {

using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;
using TimingField = page_load_metrics::PageLoadMetricsTestWaiter::TimingField;

// Paths that match Chromium's captcha URL patterns. Chrome test data does not
// include these, so serve a small HTML document instead of 404.
std::unique_ptr<HttpResponse> HandleCaptchaPath(const HttpRequest& request) {
  const bool is_captcha_path =
      base::StartsWith(request.relative_url, "/recaptcha/") ||
      base::StartsWith(request.relative_url, "/captcha/") ||
      base::StartsWith(request.relative_url, "/cdn-cgi/");
  if (!is_captcha_path) {
    return nullptr;
  }

  auto response = std::make_unique<BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content_type("text/html");
  response->set_content("<html><body>captcha</body></html>");
  return response;
}

}  // namespace

class CaptchaMetricsBrowserTestBase : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_https_test_server().SetCertHostnames(
        {"www.google.com", "example.com", "www.hcaptcha.com",
         "challenges.cloudflare.com"});
    embedded_https_test_server().RegisterRequestHandler(
        base::BindRepeating(&HandleCaptchaPath));
    ASSERT_TRUE(embedded_https_test_server().Start());
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  GURL GetURL(std::string_view host, std::string_view path) {
    return embedded_https_test_server().GetURL(host, path);
  }

  std::unique_ptr<page_load_metrics::PageLoadMetricsTestWaiter> CreateWaiter(
      content::WebContents* contents = nullptr) {
    return std::make_unique<page_load_metrics::PageLoadMetricsTestWaiter>(
        contents ? contents : web_contents(), "captcha-metrics-waiter");
  }

  void NavigateAndWaitForLoad(const GURL& url,
                              content::WebContents* contents = nullptr) {
    if (!contents) {
      contents = web_contents();
    }
    auto waiter = CreateWaiter(contents);
    waiter->AddPageExpectation(TimingField::kLoadEvent);
    ASSERT_TRUE(content::NavigateToURL(contents, url));
    waiter->Wait();
  }

  std::unique_ptr<content::WebContents> CreateIncognitoWebContents() {
    Profile* otr_profile =
        chrome_test_utils::GetProfile(this)->GetPrimaryOTRProfile(
            /*create_if_needed=*/true);
    auto contents = content::WebContents::Create(
        content::WebContents::CreateParams(otr_profile));
    InitializePageLoadMetricsForWebContents(contents.get());
    return contents;
  }

  GURL GoogleCaptchaUrl() {
    return GetURL("www.google.com", "/recaptcha/api2/anchor");
  }

  GURL CloudflareCaptchaUrl() {
    return GetURL("challenges.cloudflare.com",
                  "/cdn-cgi/challenge-platform/turnstile");
  }

  GURL HCaptchaUrl() {
    return GetURL("www.hcaptcha.com", "/captcha/index.html");
  }
};

class CaptchaMetricsBrowserTest : public CaptchaMetricsBrowserTestBase {
 public:
  CaptchaMetricsBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kCaptchaMetricsCollection);
  }

  void SetUpOnMainThread() override {
    CaptchaMetricsBrowserTestBase::SetUpOnMainThread();
    ASSERT_TRUE(
        g_brave_browser_process->process_misc_metrics()->captcha_metrics());
  }

  // P3A emits on a 24h wall-clock timer. Backdate the last-report pref so
  // ReportCounts() records the samples collected during this test.
  void ReportPendingCounts() {
    g_browser_process->local_state()->SetTime(
        kMiscMetricsCaptchaLastRecordTime, base::Time::Now() - base::Days(1));
    g_brave_browser_process->process_misc_metrics()
        ->captcha_metrics()
        ->ReportCounts();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::HistogramTester histogram_tester_;
};

class CaptchaMetricsDisabledBrowserTest : public CaptchaMetricsBrowserTestBase {
 public:
  CaptchaMetricsDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kCaptchaMetricsCollection);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(CaptchaMetricsDisabledBrowserTest,
                       DoesNotCreateMetricsWhenDisabled) {
  EXPECT_FALSE(
      g_brave_browser_process->process_misc_metrics()->captcha_metrics());

  NavigateAndWaitForLoad(GoogleCaptchaUrl());

  EXPECT_TRUE(g_browser_process->local_state()
                  ->GetDict(kMiscMetricsCaptchaDictionaryPref)
                  .empty());
}

IN_PROC_BROWSER_TEST_F(CaptchaMetricsBrowserTest, DoesNotReportUntilInterval) {
  NavigateAndWaitForLoad(GoogleCaptchaUrl());

  histogram_tester_.ExpectTotalCount(kCaptchaTotalCountHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kCaptchaGoogleCountHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kCaptchaCloudflareCountHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kCaptchaHCaptchaCountHistogramName, 0);
}

IN_PROC_BROWSER_TEST_F(CaptchaMetricsBrowserTest, DoesNotRecordInIncognito) {
  auto incognito_contents = CreateIncognitoWebContents();
  ASSERT_TRUE(incognito_contents->GetBrowserContext()->IsOffTheRecord());

  NavigateAndWaitForLoad(GoogleCaptchaUrl(), incognito_contents.get());

  EXPECT_TRUE(g_browser_process->local_state()
                  ->GetDict(kMiscMetricsCaptchaDictionaryPref)
                  .empty());

  ReportPendingCounts();
  histogram_tester_.ExpectUniqueSample(kCaptchaTotalCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaGoogleCountHistogramName, 0, 1);
}

IN_PROC_BROWSER_TEST_F(CaptchaMetricsBrowserTest, RecordsMainFrameGoogle) {
  NavigateAndWaitForLoad(GoogleCaptchaUrl());
  ReportPendingCounts();

  histogram_tester_.ExpectUniqueSample(kCaptchaTotalCountHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaGoogleCountHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaCloudflareCountHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kCaptchaHCaptchaCountHistogramName, 0,
                                       1);
}

IN_PROC_BROWSER_TEST_F(CaptchaMetricsBrowserTest, DoesNotRecordNonCaptcha) {
  NavigateAndWaitForLoad(GetURL("example.com", "/simple.html"));
  ReportPendingCounts();

  histogram_tester_.ExpectUniqueSample(kCaptchaTotalCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaGoogleCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaCloudflareCountHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kCaptchaHCaptchaCountHistogramName, 0,
                                       1);
}

IN_PROC_BROWSER_TEST_F(CaptchaMetricsBrowserTest, RecordsSubframeProviders) {
  auto waiter = CreateWaiter();
  waiter->AddPageExpectation(TimingField::kLoadEvent);
  waiter->AddSubframeNavigationExpectation();
  ASSERT_TRUE(content::NavigateToURL(web_contents(),
                                     GetURL("example.com", "/iframe.html")));
  waiter->Wait();

  const GURL subframe_urls[] = {GoogleCaptchaUrl(), CloudflareCaptchaUrl(),
                                HCaptchaUrl()};
  for (const GURL& url : subframe_urls) {
    waiter->AddSubframeNavigationExpectation();
    ASSERT_TRUE(content::NavigateIframeToURL(web_contents(), "test", url));
    waiter->Wait();
  }

  ReportPendingCounts();

  // total=3 → 3-5, one of each provider.
  histogram_tester_.ExpectUniqueSample(kCaptchaTotalCountHistogramName, 3, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaGoogleCountHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaCloudflareCountHistogramName, 1,
                                       1);
  histogram_tester_.ExpectUniqueSample(kCaptchaHCaptchaCountHistogramName, 1,
                                       1);
}

IN_PROC_BROWSER_TEST_F(CaptchaMetricsBrowserTest,
                       SkipsSubframesOnFullPageCaptcha) {
  auto waiter = CreateWaiter();
  waiter->AddPageExpectation(TimingField::kLoadEvent);
  waiter->AddSubframeNavigationExpectation();
  ASSERT_TRUE(content::NavigateToURL(
      web_contents(), GetURL("challenges.cloudflare.com", "/iframe.html")));
  waiter->Wait();

  ReportPendingCounts();

  // Main frame matches; same-origin iframe is skipped to avoid double-counting.
  histogram_tester_.ExpectUniqueSample(kCaptchaTotalCountHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaCloudflareCountHistogramName, 1,
                                       1);
}

}  // namespace misc_metrics
