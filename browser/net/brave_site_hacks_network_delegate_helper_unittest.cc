/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"

namespace {

class BraveSiteHacksNetworkDelegateHelperTest: public testing::Test {
 public:
  BraveSiteHacksNetworkDelegateHelperTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        context_(new net::TestURLRequestContext(true)) {
  }
  ~BraveSiteHacksNetworkDelegateHelperTest() override {}
  void SetUp() override {
    context_->Init();
  }
  net::TestURLRequestContext* context() { return context_.get(); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<net::TestURLRequestContext> context_;
};

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, ForbesWithCookieHeader) {
  GURL url("https://www.forbes.com");
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);
  net::HttpRequestHeaders headers;
  headers.SetHeader(kCookieHeader, "name=value; name2=value2; name3=value3");
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
      brave_request_info);
  brave::ResponseCallback callback;
  int ret = brave::OnBeforeStartTransaction_SiteHacksWork(request.get(),
      &headers, callback, brave_request_info);
  std::string cookies;
  headers.GetHeader(kCookieHeader, &cookies);
  EXPECT_TRUE(cookies.find(std::string("; ") + kForbesExtraCookies)
      != std::string::npos);
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, ForbesWithoutCookieHeader) {
  GURL url("https://www.forbes.com/prime_numbers/573259391");
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);
  net::HttpRequestHeaders headers;
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
      brave_request_info);
  brave::ResponseCallback callback;
  int ret = brave::OnBeforeStartTransaction_SiteHacksWork(request.get(),
      &headers, callback, brave_request_info);
  std::string cookies;
  headers.GetHeader(kCookieHeader, &cookies);
  EXPECT_TRUE(cookies.find(kForbesExtraCookies) != std::string::npos);
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, NotForbesNoCookieChange) {
  GURL url("https://www.brave.com/prime_numbers/573259391");
  net::TestDelegate test_delegate;
  std::unique_ptr<net::URLRequest> request =
      context()->CreateRequest(url, net::IDLE, &test_delegate,
                               TRAFFIC_ANNOTATION_FOR_TESTS);
  net::HttpRequestHeaders headers;
  std::string expected_cookies = "name=value; name2=value2; name3=value3";
  headers.SetHeader(kCookieHeader, "name=value; name2=value2; name3=value3");
  std::shared_ptr<brave::BraveRequestInfo>
      brave_request_info(new brave::BraveRequestInfo());
  brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
      brave_request_info);
  brave::ResponseCallback callback;
  int ret = brave::OnBeforeStartTransaction_SiteHacksWork(request.get(),
      &headers, callback, brave_request_info);
  std::string cookies;
  headers.GetHeader(kCookieHeader, &cookies);
  EXPECT_STREQ(cookies.c_str(), expected_cookies.c_str());
  EXPECT_EQ(ret, net::OK);
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, UAWhitelistedTest) {
  std::vector<GURL> urls({
    GURL("https://adobe.com"),
    GURL("https://adobe.com/something"),
    GURL("https://duckduckgo.com"),
    GURL("https://duckduckgo.com/something"),
    GURL("https://brave.com"),
    GURL("https://brave.com/something"),
    GURL("https://netflix.com"),
    GURL("https://netflix.com/something"),
    GURL("https://a.adobe.com"),
    GURL("https://a.duckduckgo.com"),
    GURL("https://a.brave.com"),
    GURL("https://a.netflix.com"),
    GURL("https://a.adobe.com/something"),
    GURL("https://a.duckduckgo.com/something"),
    GURL("https://a.brave.com/something"),
    GURL("https://a.netflix.com/something")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    net::HttpRequestHeaders headers;
    headers.SetHeader(kUserAgentHeader,
        "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
    std::shared_ptr<brave::BraveRequestInfo>
        brave_request_info(new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
        brave_request_info);
    brave::ResponseCallback callback;
    int ret = brave::OnBeforeStartTransaction_SiteHacksWork(request.get(),
        &headers, callback, brave_request_info);
    std::string user_agent;
    headers.GetHeader(kUserAgentHeader, &user_agent);
    EXPECT_EQ(ret, net::OK);
    EXPECT_STREQ(user_agent.c_str(),
        "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Brave Chrome/33.0.1750.117 Safari/537.36");
  });
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, NOTUAWhitelistedTest) {
  std::vector<GURL> urls({
    GURL("https://brianbondy.com"),
    GURL("https://bravecombo.com"),
    GURL("https://brave.example.com")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    net::HttpRequestHeaders headers;
    headers.SetHeader(kUserAgentHeader,
        "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
    std::shared_ptr<brave::BraveRequestInfo>
        brave_request_info(new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
        brave_request_info);
    brave::ResponseCallback callback;
    int ret = brave::OnBeforeStartTransaction_SiteHacksWork(request.get(),
        &headers, callback, brave_request_info);
    std::string user_agent;
    headers.GetHeader(kUserAgentHeader, &user_agent);
    EXPECT_EQ(ret, net::OK);
    EXPECT_STREQ(user_agent.c_str(),
        "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
  });
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, ReferrerPreserved) {
  std::vector<GURL> urls({
    GURL("https://brianbondy.com/7"),
    GURL("https://www.brianbondy.com/5"),
    GURL("https://brian.bondy.brianbondy.com")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    net::HttpRequestHeaders headers;
    std::string original_referrer = "https://hello.brianbondy.com/about";
    request->SetReferrer(original_referrer);

    std::shared_ptr<brave::BraveRequestInfo>
        brave_request_info(new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
        brave_request_info);
    brave::ResponseCallback callback;
    int ret = brave::OnBeforeURLRequest_SiteHacksWork(callback,
        brave_request_info);
    EXPECT_EQ(ret, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_STREQ(request->referrer().c_str(), original_referrer.c_str());
  });
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, ReferrerCleared) {
  std::vector<GURL> urls({
    GURL("https://digg.com/7"),
    GURL("https://slashdot.org/5"),
    GURL("https://bondy.brian.org")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);

    std::string original_referrer = "https://hello.brianbondy.com/about";
    request->SetReferrer(original_referrer);

    std::shared_ptr<brave::BraveRequestInfo>
        brave_request_info(new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
        brave_request_info);
    brave::ResponseCallback callback;
    int ret = brave::OnBeforeURLRequest_SiteHacksWork(callback,
        brave_request_info);
    EXPECT_EQ(ret, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_STREQ(request->referrer().c_str(), url.GetOrigin().spec().c_str());
  });
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest,
    ReferrerWouldBeClearedButExtensionSite) {
  std::vector<GURL> urls({
    GURL("https://digg.com/7"),
    GURL("https://slashdot.org/5"),
    GURL("https://bondy.brian.org")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(url, net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);
    request->set_site_for_cookies(
        GURL("chrome-extension://aemmndcbldboiebfnladdacbdfmadadm/test.html"));
    std::string original_referrer = "https://hello.brianbondy.com/about";
    request->SetReferrer(original_referrer);

    std::shared_ptr<brave::BraveRequestInfo>
        brave_request_info(new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
        brave_request_info);
    brave::ResponseCallback callback;
    int ret = brave::OnBeforeURLRequest_SiteHacksWork(callback,
        brave_request_info);
    EXPECT_EQ(ret, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_STREQ(request->referrer().c_str(), original_referrer.c_str());
  });
}

}  // namespace
