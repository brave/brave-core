/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>
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
    int ret = brave::OnBeforeStartTransaction_SiteHacksWork(
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
    int ret = brave::OnBeforeStartTransaction_SiteHacksWork(
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
    EXPECT_EQ(brave_request_info->new_referrer, url.GetOrigin().spec());
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

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, QueryStringUntouched) {
  const std::vector<const std::string> urls({
      "https://example.com/",
      "https://example.com/?",
      "https://example.com/?+%20",
      "https://user:pass@example.com/path/file.html?foo=1#fragment",
      "http://user:pass@example.com/path/file.html?foo=1&bar=2#fragment",
      "https://example.com/?file=https%3A%2F%2Fexample.com%2Ftest.pdf",
      "https://example.com/?title=1+2&caption=1%202",
      "https://example.com/?foo=1&&bar=2#fragment",
      "https://example.com/?foo&bar=&#fragment",
      "https://example.com/?foo=1&fbcid=no&gcid=no&mc_cid=no&bar=&#frag",
      "https://example.com/?fbclid=&gclid&=mc_eid&msclkid=",
      "https://example.com/?value=fbclid=1&not-gclid=2&foo+mc_eid=3",
      "https://example.com/?+fbclid=1",
      "https://example.com/?%20fbclid=1",
      "https://example.com/#fbclid=1",
  });
  for (const auto& url : urls) {
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request = context()->CreateRequest(
        GURL(url), net::IDLE, &test_delegate, TRAFFIC_ANNOTATION_FOR_TESTS);

    std::shared_ptr<brave::BraveRequestInfo> brave_request_info(
        new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                                brave_request_info);
    brave::ResponseCallback callback;
    int ret =
        brave::OnBeforeURLRequest_SiteHacksWork(callback, brave_request_info);
    EXPECT_EQ(ret, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_EQ(request->url(), GURL(url));
  }
}

TEST_F(BraveSiteHacksNetworkDelegateHelperTest, QueryStringFiltered) {
  const std::vector<const std::pair<const std::string, const std::string>> urls(
      {
          // { original url, expected url after filtering }
          {"https://example.com/?fbclid=1234", "https://example.com/"},
          {"https://example.com/?fbclid=1234&", "https://example.com/"},
          {"https://example.com/?&fbclid=1234", "https://example.com/"},
          {"https://example.com/?gclid=1234", "https://example.com/"},
          {"https://example.com/?fbclid=0&gclid=1&msclkid=a&mc_eid=a1",
           "https://example.com/"},
          {"https://example.com/?fbclid=&foo=1&bar=2&gclid=abc",
           "https://example.com/?fbclid=&foo=1&bar=2"},
          {"https://example.com/?fbclid=&foo=1&gclid=1234&bar=2",
           "https://example.com/?fbclid=&foo=1&bar=2"},
          {"http://u:p@example.com/path/file.html?foo=1&fbclid=abcd#fragment",
           "http://u:p@example.com/path/file.html?foo=1#fragment"},
          // Obscure edge cases that break most parsers:
          {"https://example.com/?fbclid&foo&&gclid=2&bar=&%20",
           "https://example.com/?fbclid&foo&&bar=&%20"},
          {"https://example.com/?fbclid=1&1==2&=msclkid&foo=bar&&a=b=c&",
           "https://example.com/?1==2&=msclkid&foo=bar&&a=b=c&"},
          {"https://example.com/?fbclid=1&=2&?foo=yes&bar=2+",
           "https://example.com/?=2&?foo=yes&bar=2+"},
          {"https://example.com/?fbclid=1&a+b+c=some%20thing&1%202=3+4",
           "https://example.com/?a+b+c=some%20thing&1%202=3+4"},
      });
  for (const auto& pair : urls) {
    net::TestDelegate test_delegate;
    std::unique_ptr<net::URLRequest> request =
        context()->CreateRequest(GURL(pair.first), net::IDLE, &test_delegate,
                                 TRAFFIC_ANNOTATION_FOR_TESTS);

    std::shared_ptr<brave::BraveRequestInfo> brave_request_info(
        new brave::BraveRequestInfo());
    brave::BraveRequestInfo::FillCTXFromRequest(request.get(),
                                                brave_request_info);
    brave::ResponseCallback callback;
    int ret =
        brave::OnBeforeURLRequest_SiteHacksWork(callback, brave_request_info);
    EXPECT_EQ(ret, net::OK);
    EXPECT_EQ(brave_request_info->new_url_spec, pair.second);
  }
}

}  // namespace
