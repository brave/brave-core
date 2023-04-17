/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/legacy/bat_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatHelperTest.*

namespace brave_rewards::internal {

TEST(BatHelperTest, HasSameDomainAndPath) {
  std::string url("https://k8923479-sub.cdn.ttvwn.net/v1/segment/");
  std::string url_portion("ttvwn.net");
  std::string path("/v1/segment");
  // regular url
  bool result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, true);

  // empty url with portion
  url = std::string();
  result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, false);

  // url with empty portion and path
  url = "https://k8923479-sub.cdn.ttvwn.net/v1/segment/";
  url_portion = std::string();
  result = HasSameDomainAndPath(url, url_portion, std::string());
  ASSERT_EQ(result, false);

  // all empty
  url = url_portion = path = std::string();
  result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, false);

  // portion not all part of host
  url = "https://k8923479-sub.cdn.ttvwn.net/v1/segment/";
  url_portion = "cdn.ttvwn.net";
  path = "/v1/seg";
  result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, true);

  // domain is malicious
  url = "https://www.baddomain.com/k8923479-sub.cdn.ttvwn.net/v1/segment/";
  result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, false);

  // portion without leading . matched to malicious
  url_portion = "cdn.ttvwn.net/v1/seg";
  result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, false);

  // domain is malicious
  url =
      "https://www.baddomain.com/query?=k8923479-sub.cdn.ttvwn.net/v1/segment/";
  result = HasSameDomainAndPath(url, url_portion, path);
  ASSERT_EQ(result, false);
}

}  // namespace brave_rewards::internal
