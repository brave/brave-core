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
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave::ResponseCallback;

TEST(BraveSiteHacksNetworkDelegateHelperTest, UAWhitelistedTest) {
  const std::vector<const GURL> urls(
      {GURL("https://duckduckgo.com"), GURL("https://duckduckgo.com/something"),
       GURL("https://netflix.com"), GURL("https://netflix.com/something"),
       GURL("https://a.duckduckgo.com"),
       GURL("https://a.netflix.com"),
       GURL("https://a.duckduckgo.com/something"),
       GURL("https://a.netflix.com/something")});
  for (const auto& url : urls) {
    net::HttpRequestHeaders headers;
    headers.SetHeader(kUserAgentHeader,
                      "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
                      "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    int rc = brave::OnBeforeStartTransaction_SiteHacksWork(
        &headers, ResponseCallback(), brave_request_info);
    std::string user_agent;
    headers.GetHeader(kUserAgentHeader, &user_agent);
    EXPECT_EQ(rc, net::OK);
    EXPECT_EQ(user_agent,
              "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
              "(KHTML, like Gecko) Brave Chrome/33.0.1750.117 Safari/537.36");
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, ChangeUAOnlyOnce) {
  const GURL whitelisted_url("https://netflix.com/");
  net::HttpRequestHeaders headers;
  headers.SetHeader(kUserAgentHeader,
                    "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
                    "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
  auto brave_request_info =
      std::make_shared<brave::BraveRequestInfo>(whitelisted_url);

  // Check once.
  int rc = brave::OnBeforeStartTransaction_SiteHacksWork(
      &headers, ResponseCallback(), brave_request_info);
  std::string user_agent;
  headers.GetHeader(kUserAgentHeader, &user_agent);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(user_agent,
            "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Brave Chrome/33.0.1750.117 Safari/537.36");

  // Check twice.
  rc = brave::OnBeforeStartTransaction_SiteHacksWork(
      &headers, ResponseCallback(), brave_request_info);
  headers.GetHeader(kUserAgentHeader, &user_agent);
  EXPECT_EQ(rc, net::OK);
  EXPECT_EQ(user_agent,
            "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
            "(KHTML, like Gecko) Brave Chrome/33.0.1750.117 Safari/537.36");
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, NOTUAWhitelistedTest) {
  const std::vector<const GURL> urls({GURL("https://brianbondy.com"),
                                      GURL("https://bravecombo.com"),
                                      GURL("https://brave.example.com")});
  for (const auto& url : urls) {
    net::HttpRequestHeaders headers;
    headers.SetHeader(kUserAgentHeader,
                      "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
                      "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    int rc = brave::OnBeforeStartTransaction_SiteHacksWork(
        &headers, ResponseCallback(), brave_request_info);
    std::string user_agent;
    headers.GetHeader(kUserAgentHeader, &user_agent);
    EXPECT_EQ(rc, net::OK);
    EXPECT_EQ(user_agent,
              "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 "
              "(KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, ReferrerPreserved) {
  const std::vector<const GURL> urls(
      {GURL("https://brianbondy.com/7"), GURL("https://www.brianbondy.com/5"),
       GURL("https://brian.bondy.brianbondy.com")});
  for (const auto& url : urls) {
    net::HttpRequestHeaders headers;
    const GURL original_referrer("https://hello.brianbondy.com/about");

    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->referrer = original_referrer;
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set.
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_EQ(brave_request_info->referrer, original_referrer);
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, ReferrerTruncated) {
  const std::vector<const GURL> urls({GURL("https://digg.com/7"),
                                      GURL("https://slashdot.org/5"),
                                      GURL("https://bondy.brian.org")});
  for (const auto& url : urls) {
    const GURL original_referrer("https://hello.brianbondy.com/about");

    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->referrer = original_referrer;
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set.
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_TRUE(brave_request_info->new_referrer.has_value());
    EXPECT_EQ(brave_request_info->new_referrer.value(),
              original_referrer.GetOrigin().spec());
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest,
     ReferrerWouldBeClearedButExtensionSite) {
  const std::vector<const GURL> urls({GURL("https://digg.com/7"),
                                      GURL("https://slashdot.org/5"),
                                      GURL("https://bondy.brian.org")});
  for (const auto& url : urls) {
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave_request_info->tab_origin =
        GURL("chrome-extension://aemmndcbldboiebfnladdacbdfmadadm/");
    const GURL original_referrer("https://hello.brianbondy.com/about");
    brave_request_info->referrer = original_referrer;

    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
    EXPECT_EQ(brave_request_info->referrer, original_referrer);
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, QueryStringUntouched) {
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
      "https://example.com/1;k=v;&a=b&c=d&gclid=1234;%3fhttp://ad.co/?e=f&g=1",
  });
  for (const auto& url : urls) {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(GURL(url));
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, QueryStringFiltered) {
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
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(GURL(pair.first));
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    EXPECT_EQ(brave_request_info->new_url_spec, pair.second);
  }
}
