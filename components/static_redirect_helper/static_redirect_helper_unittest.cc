// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/static_redirect_helper/static_redirect_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave {

class StaticRedirectHelperUnitTest : public testing::Test {
 public:
  StaticRedirectHelperUnitTest() = default;
  ~StaticRedirectHelperUnitTest() override = default;
};

TEST_F(StaticRedirectHelperUnitTest, FaviconServiceMatch) {
  GURL old_url = GURL(
      "https://t0.gstatic.com/"
      "faviconV2?client=chrome&nfrp=2&check_seen=true&size=32&min_size=16&max_"
      "size=256&fallback_opts=TYPE,SIZE,URL&url=https://search.brave.com/");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(
      new_url.spec(),
      "https://favicons.proxy.brave.com/"
      "faviconV2?client=chrome&nfrp=2&check_seen=true&size=32&min_size=16&max_"
      "size=256&fallback_opts=TYPE,SIZE,URL&url=https://search.brave.com/");
}

TEST_F(StaticRedirectHelperUnitTest, FaviconServiceMatchNoParams) {
  GURL old_url = GURL("https://t0.gstatic.com/faviconV2");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "https://favicons.proxy.brave.com/faviconV2");
}

TEST_F(StaticRedirectHelperUnitTest, DontMatchGstaticImages) {
  GURL old_url = GURL(
      "https://t0.gstatic.com/"
      "images?client=chrome&nfrp=2&check_seen=true&size=32&min_size=16&max_"
      "size=256&fallback_opts=TYPE,SIZE,URL&url=https://search.brave.com/");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "");
}

TEST_F(StaticRedirectHelperUnitTest, FaviconServicePartialMatch) {
  GURL old_url = GURL("https://t0.gstatic.com/faviconV");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "");
}

TEST_F(StaticRedirectHelperUnitTest, FaviconServiceCloseButNoMatch) {
  GURL old_url = GURL("https://t0.gstatic.com/faviconV1");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "");
}

}  // namespace brave
