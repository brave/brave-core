/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave::GetPolyfillForAdBlock;

namespace {

TEST(BraveAdBlockTPNetworkDelegateHelperTest, NoChangeURL) {
  GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave::ResponseCallback callback;
  int ret = OnBeforeURLRequest_AdBlockTPPreWork(callback, brave_request_info);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  EXPECT_EQ(ret, net::OK);
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, EmptyRequestURL) {
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(GURL());
  brave::ResponseCallback callback;
  int ret = OnBeforeURLRequest_AdBlockTPPreWork(callback,
      brave_request_info);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  EXPECT_EQ(ret, net::OK);
}


TEST(BraveAdBlockTPNetworkDelegateHelperTest, RedirectsToStubs) {
  std::vector<GURL> urls({
    GURL(kGoogleTagManagerPattern),
    GURL(kGoogleTagServicesPattern)
  });
  for(const auto& url : urls) {
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave::ResponseCallback callback;
    int ret = OnBeforeURLRequest_AdBlockTPPreWork(callback,
        brave_request_info);
    EXPECT_EQ(ret, net::OK);
    EXPECT_TRUE(GURL(brave_request_info->new_url_spec).SchemeIs("data"));
  }
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, Blocking) {
  std::vector<GURL> urls({
      GURL("https://pdfjs.robwu.nl/ping"),
    });
  for(const auto& url : urls) {
    auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
    brave::ResponseCallback callback;
    int ret = OnBeforeURLRequest_AdBlockTPPreWork(callback, brave_request_info);
    EXPECT_STREQ(brave_request_info->new_url_spec.c_str(), kEmptyDataURI);
    EXPECT_EQ(ret, net::OK);
  }
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, GetPolyfill) {
  GURL tab_origin("https://test.com");
  GURL google_analytics_url(kGoogleAnalyticsPattern);
  GURL tag_manager_url(kGoogleTagManagerPattern);
  GURL tag_services_url(kGoogleTagServicesPattern);
  GURL normal_url("https://a.com");
  std::string out_url_spec;
  // Shields up, block ads, google analytics should get polyfill
  ASSERT_TRUE(GetPolyfillForAdBlock(true, false, tab_origin,
      google_analytics_url, &out_url_spec));
  // Shields up, block ads, tag manager should get polyfill
  ASSERT_TRUE(GetPolyfillForAdBlock(true, false, tab_origin, tag_manager_url,
      &out_url_spec));
  // Shields up, block ads, tag services should get polyfill
  ASSERT_TRUE(GetPolyfillForAdBlock(true, false, tab_origin, tag_services_url,
      &out_url_spec));
  // Shields up, block ads, normal URL should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(true, false, tab_origin, normal_url,
      &out_url_spec));

  // Shields up, allow ads, google analytics should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(true, true, tab_origin,
      google_analytics_url, &out_url_spec));
  // Shields up, allow ads, tag manager should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(true, true, tab_origin, tag_manager_url,
      &out_url_spec));
  // Shields up, allow ads, tag services should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(true, true, tab_origin, tag_services_url,
      &out_url_spec));
  // Shields up, allow ads, normal URL should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(true, true, tab_origin, normal_url,
      &out_url_spec));

  // Shields down, allow ads, google analytics should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, true, tab_origin,
      google_analytics_url, &out_url_spec));
  // Shields down, allow ads, tag manager should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, true, tab_origin, tag_manager_url,
      &out_url_spec));
  // Shields down, allow ads, tag services should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, true, tab_origin, tag_services_url,
      &out_url_spec));
  // Shields down, allow ads, normal URL should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, true, tab_origin, normal_url,
      &out_url_spec));

  // Shields down, block ads, google analytics should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, false, tab_origin,
      google_analytics_url, &out_url_spec));
  // Shields down, block ads, tag manager should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, false, tab_origin, tag_manager_url,
      &out_url_spec));
  // Shields down, block ads, tag services should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, false, tab_origin,
      tag_services_url, &out_url_spec));
  // Shields down, block ads, normal URL should NOT get polyfill
  ASSERT_FALSE(GetPolyfillForAdBlock(false, false, tab_origin, normal_url,
      &out_url_spec));
}

}  // namespace
