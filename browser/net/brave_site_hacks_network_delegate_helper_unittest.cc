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
#include "brave/components/constants/network_constants.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_job.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using brave::ResponseCallback;

TEST(BraveSiteHacksNetworkDelegateHelperTest, UANotAllowedTest) {
  const std::vector<GURL> urls({GURL("https://brianbondy.com"),
                                GURL("https://bravecombo.com"),
                                GURL("https://brave.example.com"),
                                GURL("https://a.netflix.com/something"),
                                GURL("https://a.duckduckgo.com/something")});
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
  const std::vector<GURL> urls({GURL("https://brianbondy.com/7"),
                                GURL("https://www.brianbondy.com/5"),
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
  const std::vector<GURL> urls({GURL("https://digg.com/7"),
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
              url::Origin::Create(original_referrer).GetURL());
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest,
     ReferrerWouldBeClearedButExtensionSite) {
  const std::vector<GURL> urls({GURL("https://digg.com/7"),
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

TEST(BraveSiteHacksNetworkDelegateHelperTest, OnionReferrerStripped) {
  const GURL original_referrer(
      "https://"
      "brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd.onion/");
  const GURL destination("https://brave.com");

  // Cross-origin request from a .onion gets empty referrer.
  auto url1 = net::URLRequestJob::ComputeReferrerForPolicy(
      net::ReferrerPolicy::NEVER_CLEAR, original_referrer, destination);
  EXPECT_EQ(url1, GURL());

  // Cross-origin request to a .onion gets normal referrer.
  auto url2 = net::URLRequestJob::ComputeReferrerForPolicy(
      net::ReferrerPolicy::NEVER_CLEAR, destination, original_referrer);
  EXPECT_EQ(url2, destination);
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, QueryStringUntouched) {
  const std::vector<std::string> urls(
      {"https://example.com/",
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
       "https://example.com/?__ss=1234-abcd",
       "https://example.com/?mkt_tok=123&mkt_unsubscribe=1",
       "https://example.com/?mkt_unsubscribe=1&fake_param=abc&mkt_tok=123",
       "https://example.com/Unsubscribe.html?fake_param=abc&mkt_tok=123"});
  for (const auto& url : urls) {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(GURL(url));
    brave_request_info->initiator_url =
        GURL("https://example.net");  // cross-site
    brave_request_info->method = "GET";
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, QueryStringExempted) {
  const GURL tracking_url("https://example.com/?fbclid=1");

  const std::string initiators[] = {
      "https://example.com/path",      // Same-origin
      "https://sub.example.com/path",  // Same-site
  };

  for (const auto& initiator : initiators) {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(tracking_url);
    brave_request_info->initiator_url = GURL(initiator);
    brave_request_info->method = "GET";
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  }

  // Internal redirect
  {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(tracking_url);
    brave_request_info->initiator_url =
        GURL("https://example.net");  // cross-site
    brave_request_info->method = "GET";
    brave_request_info->internal_redirect = true;
    brave_request_info->redirect_source =
        GURL("https://example.org");  // cross-site
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  }

  // POST requests
  {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(tracking_url);
    brave_request_info->initiator_url =
        GURL("https://example.net");  // cross-site
    brave_request_info->method = "POST";
    brave_request_info->redirect_source =
        GURL("https://example.org");  // cross-site
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  }

  // Same-site redirect
  {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(tracking_url);
    brave_request_info->initiator_url =
        GURL("https://example.net");  // cross-site
    brave_request_info->method = "GET";
    brave_request_info->redirect_source =
        GURL("https://sub.example.com");  // same-site
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    // new_url should not be set
    EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  }
}

TEST(BraveSiteHacksNetworkDelegateHelperTest, QueryStringFiltered) {
  const std::vector<std::pair<std::string, std::string>> urls(
      {// { original url, expected url after filtering ("" means unchanged) }
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
       {"https://example.com/?__s=1234-abcd", "https://example.com/"},
       // Obscure edge cases that break most parsers:
       {"https://example.com/?fbclid&foo&&gclid=2&bar=&%20",
        "https://example.com/?fbclid&foo&&bar=&%20"},
       {"https://example.com/?fbclid=1&1==2&=msclkid&foo=bar&&a=b=c&",
        "https://example.com/?1==2&=msclkid&foo=bar&&a=b=c&"},
       {"https://example.com/?fbclid=1&=2&?foo=yes&bar=2+",
        "https://example.com/?=2&?foo=yes&bar=2+"},
       {"https://example.com/?fbclid=1&a+b+c=some%20thing&1%202=3+4",
        "https://example.com/?a+b+c=some%20thing&1%202=3+4"},
       // Conditional query parameter stripping
       {"https://example.com/?igshid=1234", ""},
       {"https://www.instagram.com/?igshid=1234", "https://www.instagram.com/"},
       {"https://brave.com/?ref_src=example.com", ""},
       {"https://twitter.com/?ref_src=example.com", "https://twitter.com/"},
       {"https://x.com/?ref_src=example.com", "https://x.com/"},
       {"https://example.com/?mkt_tok=123&foo=bar&mkt_unsubscribe=1", ""},
       {"https://example.com/index.php/email/emailWebview?mkt_tok=1234&foo=bar",
        ""},
       {"https://example.com/?mkt_tok=123&foo=bar",
        "https://example.com/?foo=bar"}});
  for (const auto& pair : urls) {
    auto brave_request_info =
        std::make_shared<brave::BraveRequestInfo>(GURL(pair.first));
    brave_request_info->initiator_url =
        GURL("https://example.net");  // cross-site
    brave_request_info->method = "GET";
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    EXPECT_EQ(brave_request_info->new_url_spec, pair.second);
  }

  // Cross-site redirect
  {
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(
        GURL("https://example.com/?fbclid=1"));
    brave_request_info->initiator_url =
        GURL("https://example.com");  // same-origin
    brave_request_info->method = "GET";
    brave_request_info->redirect_source =
        GURL("https://example.net");  // cross-site
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    EXPECT_EQ(brave_request_info->new_url_spec, "https://example.com/");
  }

  // Direct navigation
  {
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(
        GURL("https://example.com/?fbclid=2"));
    brave_request_info->initiator_url = GURL();
    brave_request_info->method = "GET";
    int rc = brave::OnBeforeURLRequest_SiteHacksWork(ResponseCallback(),
                                                     brave_request_info);
    EXPECT_EQ(rc, net::OK);
    EXPECT_EQ(brave_request_info->new_url_spec, "https://example.com/");
  }
}
