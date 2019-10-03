/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_block_safebrowsing_urls.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/net/url_context.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

const char kInvalidUrl[] = "https://no-thanks.invalid";

class BraveBlockReportingUrlsHelperTest : public testing::Test {
 public:
  BraveBlockReportingUrlsHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {}
  ~BraveBlockReportingUrlsHelperTest() override {}
  void SetUp() override { context_->Init(); }
  net::TestURLRequestContext* context() { return context_.get(); }
  void CheckUrl(const std::string& test_url,
                const char* expected_url,
                int expected_error);

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

void BraveBlockReportingUrlsHelperTest::CheckUrl(const std::string& test_url,
                                                 const char* expected_url,
                                                 int expected_error) {
  net::TestDelegate test_delegate;
  GURL url(test_url);
  std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
      url, net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);
  std::shared_ptr<brave::BraveRequestInfo> before_url_context(
      new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                              before_url_context);
  brave::ResponseCallback callback;
  GURL new_url;
  int ret =
      brave::OnBeforeURLRequest_BlockSafeBrowsingReportingURLs(url, &new_url);
  EXPECT_EQ(new_url, GURL(expected_url));
  EXPECT_EQ(ret, expected_error);
}

TEST_F(BraveBlockReportingUrlsHelperTest, PreserveNormalUrls) {
  const std::vector<const std::string> normalUrls({
      "https://brave.com/",
      "https://safebrowsing.google.com/safebrowsing",
      "https://safebrowsing.googleapis.com/v4",
  });

  for (const auto& url : normalUrls) {
    CheckUrl(url, "", net::OK);
  }
}

TEST_F(BraveBlockReportingUrlsHelperTest, CancelReportingUrl) {
  const std::vector<const std::string> reportingUrls({
      "https://sb-ssl.google.com/safebrowsing/clientreport/download",
      "https://sb-ssl.google.com/safebrowsing/clientreport/chrome-reset",
      "https://sb-ssl.google.com/safebrowsing/clientreport/incident",
      "https://sb-ssl.google.com/safebrowsing/clientreport/login",
      "https://sb-ssl.google.com/safebrowsing/clientreport/phishing",
      "https://sb-ssl.google.com/safebrowsing/clientreport/malware-check",
      "https://safebrowsing.google.com/safebrowsing/uploads/webprotect",
      "https://safebrowsing.google.com/safebrowsing/report",
      "https://safebrowsing.google.com/safebrowsing/clientreport/malware",
      "https://safebrowsing.google.com/safebrowsing/uploads/chrome",
      "https://safebrowsing.google.com/safebrowsing/clientreport/crx-list-info",
  });

  for (const auto& url : reportingUrls) {
    CheckUrl(url, kInvalidUrl, net::ERR_ABORTED);
  }
}

}  // namespace
