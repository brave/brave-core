/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_referrals_network_delegate_helper.h"

#include <memory>
#include <string>
#include <tuple>

#include "base/json/json_reader.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/constants/network_constants.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using brave::ResponseCallback;

TEST(BraveReferralsNetworkDelegateHelperTest, ReplaceHeadersForMatchingDomain) {
  const std::array<std::tuple<GURL, std::string>, 1> test_cases = {
      std::make_tuple<>(GURL("http://grammarly.com"), "grammarly"),
  };

  for (const auto& c : test_cases) {
    net::HttpRequestHeaders headers;
    auto request_info =
        std::make_shared<brave::BraveRequestInfo>(std::get<0>(c));

    int rc = brave::OnBeforeStartTransaction_ReferralsWork(
        &headers, brave::ResponseCallback(), request_info);

    std::string partner_header;
    headers.GetHeader("X-Brave-Partner", &partner_header);
    EXPECT_EQ(partner_header, std::get<1>(c));
    EXPECT_EQ(rc, net::OK);
  }
}

TEST(BraveReferralsNetworkDelegateHelperTest,
     NoReplaceHeadersForNonMatchingDomain) {
  const std::array<GURL, 2> test_cases = {
      GURL("https://api-sandbox.uphold.com"),
      GURL("https://www.google.com"),
  };

  for (const auto& c : test_cases) {
    net::HttpRequestHeaders headers;

    auto request_info = std::make_shared<brave::BraveRequestInfo>(c);
    int rc = brave::OnBeforeStartTransaction_ReferralsWork(
        &headers, brave::ResponseCallback(), request_info);

    EXPECT_FALSE(headers.HasHeader("X-Brave-Partner"));
    EXPECT_EQ(rc, net::OK);
  }
}
