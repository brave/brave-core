/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/browser/net/brave_stp_util.h"
#include "brave/browser/net/url_context.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/browser_task_environment.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"

using brave::RemoveTrackableSecurityHeadersForThirdParty;
using brave::TrackableSecurityHeaders;
using net::HttpResponseHeaders;


namespace {

constexpr char kFirstPartyDomain[] = "http://firstparty.com/";
constexpr char kThirdPartyDomain[] = "http://thirdparty.com/";
constexpr char kAcceptLanguageHeader[] = "Accept-Language";
constexpr char kXSSProtectionHeader[] = "X-XSS-Protection";

constexpr char kRawHeaders[] =
    "HTTP/1.0 200 OK\n"
    "Strict-Transport-Security: max-age=31557600\n"
    "Accept-Language: *\n"
    "Expect-CT: max-age=86400, enforce "
    "report-uri=\"https://foo.example/report\"\n"
    "Public-Key-Pins:"
    "pin-sha256=\"cUPcTAZWKaASuYWhhBAkE3h2+soZS7sWs=\""
    "max-age=5184000; includeSubDomains\n"
    "Public-Key-Pins-Report-Only:"
    "pin-sha256=\"cUPcTAZWKaASuYWhhBAkE3h2+soZS7sWs=\""
    "max-age=5184000; includeSubDomains"
    "report-uri=\"https://www.pkp.org/hpkp-report\"\n"
    "X-XSS-Protection: 0";

class BraveNetworkDelegateBaseTest : public testing::Test {
 public:
  BraveNetworkDelegateBaseTest()
      : task_environment_(content::BrowserTaskEnvironment::IO_MAINLOOP) {}
  ~BraveNetworkDelegateBaseTest() override = default;
  void SetUp() override {
    context_ = net::CreateTestURLRequestContextBuilder()->Build();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<net::URLRequestContext> context_;
};

TEST_F(BraveNetworkDelegateBaseTest, RemoveTrackableSecurityHeaders) {
  net::TestDelegate test_delegate;
  GURL request_url(kThirdPartyDomain);
  GURL tab_url(kFirstPartyDomain);

  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(kRawHeaders)));

  RemoveTrackableSecurityHeadersForThirdParty(request_url,
                                              url::Origin::Create(tab_url),
                                              nullptr, &headers);
  for (auto header : *TrackableSecurityHeaders()) {
    EXPECT_FALSE(headers->HasHeader(std::string(header)));
  }
  EXPECT_TRUE(headers->HasHeader(kAcceptLanguageHeader));
  EXPECT_TRUE(headers->HasHeader(kXSSProtectionHeader));
}

TEST_F(BraveNetworkDelegateBaseTest, RemoveTrackableSecurityHeadersMixedCase) {
  net::TestDelegate test_delegate;
  GURL request_url(kThirdPartyDomain);
  GURL tab_url(kFirstPartyDomain);

  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(kRawHeaders)));

  RemoveTrackableSecurityHeadersForThirdParty(request_url,
                                              url::Origin::Create(tab_url),
                                              nullptr, &headers);
  for (auto header : *TrackableSecurityHeaders()) {
    EXPECT_FALSE(headers->HasHeader(std::string(header)));
  }
  EXPECT_TRUE(headers->HasHeader(kAcceptLanguageHeader));
  EXPECT_TRUE(headers->HasHeader(kXSSProtectionHeader));
}

TEST_F(BraveNetworkDelegateBaseTest, RetainTrackableSecurityHeaders) {
  net::TestDelegate test_delegate;
  GURL request_url(kFirstPartyDomain);
  GURL tab_url(kFirstPartyDomain);

  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(kRawHeaders)));

  RemoveTrackableSecurityHeadersForThirdParty(request_url,
                                              url::Origin::Create(tab_url),
                                              nullptr, &headers);
  for (auto header : *TrackableSecurityHeaders()) {
    EXPECT_TRUE(headers->HasHeader(std::string(header)));
  }
  EXPECT_TRUE(headers->HasHeader(kAcceptLanguageHeader));
  EXPECT_TRUE(headers->HasHeader(kXSSProtectionHeader));
}

}  // namespace
