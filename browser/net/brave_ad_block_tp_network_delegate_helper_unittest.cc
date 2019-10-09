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

using brave::ResponseCallback;

TEST(BraveAdBlockTPNetworkDelegateHelperTest, NoChangeURL) {
  const GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  int rc = OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(),
                                               request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, EmptyRequestURL) {
  auto request_info = std::make_shared<brave::BraveRequestInfo>(GURL());
  int rc = OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(),
                                               request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, RedirectsToStubs) {
  const std::vector<const GURL> urls({
    GURL(kGoogleTagManagerPattern),
    GURL(kGoogleTagServicesPattern)
  });
  for (const auto& url : urls) {
    auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
    int rc = OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(),
                                                 request_info);
    EXPECT_EQ(rc, net::OK);
    EXPECT_TRUE(GURL(request_info->new_url_spec).SchemeIs("data"));
  }
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, Blocking) {
  const std::vector<const GURL> urls({
      GURL("https://pdfjs.robwu.nl/ping"),
    });
  for (const auto& url : urls) {
    auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
    int rc = OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(),
                                                 request_info);
    EXPECT_EQ(request_info->new_url_spec, kEmptyDataURI);
    EXPECT_EQ(rc, net::OK);
  }
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, GetPolyfill) {
  using brave::GetPolyfillForAdBlock;

  const GURL tab_origin("https://test.com");
  const GURL google_analytics_url(kGoogleAnalyticsPattern);
  const GURL tag_manager_url(kGoogleTagManagerPattern);
  const GURL tag_services_url(kGoogleTagServicesPattern);
  const GURL normal_url("https://a.com");

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
