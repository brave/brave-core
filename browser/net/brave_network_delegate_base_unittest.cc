/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_delegate_base.h"

#include <string>

#include "brave/browser/net/url_context.h"
#include "brave/browser/net/brave_stp_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "url/gurl.h"

using brave::RemoveTrackableSecurityHeadersForThirdParty;
using brave::TrackableSecurityHeaders;
using net::HttpResponseHeaders;


namespace {

const char kFirstPartyDomain[] = "http://firstparty.com/";
const char kThirdPartyDomain[] = "http://thirdparty.com/";
const char kAcceptLanguageHeader[] = "Accept-Language";
const char kXSSProtectionHeader[] = "X-XSS-Protection";

const char kRawHeaders[] =
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
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {}
  ~BraveNetworkDelegateBaseTest() override {}
  void SetUp() override { context_->Init(); }
  net::TestURLRequestContext* context() { return context_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

TEST_F(BraveNetworkDelegateBaseTest, RemoveTrackableSecurityHeaders) {
  net::TestDelegate test_delegate;
  GURL request_url(kThirdPartyDomain);
  GURL tab_url(kFirstPartyDomain);
  std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
      request_url, net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);

  request->set_top_frame_origin(url::Origin::Create(tab_url));

  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(kRawHeaders)));

  RemoveTrackableSecurityHeadersForThirdParty(request.get(), nullptr, &headers);
  for (auto header : *TrackableSecurityHeaders()) {
    EXPECT_FALSE(headers->HasHeader(header.as_string()));
  }
  EXPECT_TRUE(headers->HasHeader(kAcceptLanguageHeader));
  EXPECT_TRUE(headers->HasHeader(kXSSProtectionHeader));
}

TEST_F(BraveNetworkDelegateBaseTest, RemoveTrackableSecurityHeadersMixedCase) {
  net::TestDelegate test_delegate;
  GURL request_url(kThirdPartyDomain);
  GURL tab_url(kFirstPartyDomain);
  std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
      request_url, net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);

  request->set_top_frame_origin(url::Origin::Create(tab_url));

  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(kRawHeaders)));

  RemoveTrackableSecurityHeadersForThirdParty(request.get(), nullptr, &headers);
  for (auto header : *TrackableSecurityHeaders()) {
    EXPECT_FALSE(headers->HasHeader(header.as_string()));
  }
  EXPECT_TRUE(headers->HasHeader(kAcceptLanguageHeader));
  EXPECT_TRUE(headers->HasHeader(kXSSProtectionHeader));
}

TEST_F(BraveNetworkDelegateBaseTest, RetainTrackableSecurityHeaders) {
  net::TestDelegate test_delegate;
  GURL request_url(kFirstPartyDomain);
  GURL tab_url(kFirstPartyDomain);
  std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
      request_url, net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);

  request->set_top_frame_origin(url::Origin::Create(tab_url));

  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(kRawHeaders)));

  RemoveTrackableSecurityHeadersForThirdParty(request.get(), nullptr, &headers);
  for (auto header : *TrackableSecurityHeaders()) {
    EXPECT_TRUE(headers->HasHeader(header.as_string()));
  }
  EXPECT_TRUE(headers->HasHeader(kAcceptLanguageHeader));
  EXPECT_TRUE(headers->HasHeader(kXSSProtectionHeader));
}

}  // namespace
