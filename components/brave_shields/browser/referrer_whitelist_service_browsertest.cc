/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"

const char kTestDataDirectory[] = "referrer-whitelist-data";

class ReferrerWhitelistServiceTest : public BaseLocalDataFilesBrowserTest {
 public:
  // BaseLocalDataFilesBrowserTest overrides
  const char* test_data_directory() override { return kTestDataDirectory; }
  const char* embedded_test_server_directory() override { return NULL; }
  LocalDataFilesObserver* service() override {
    return g_brave_browser_process->referrer_whitelist_service();
  }

  // functions used by referrer whitelist service tests
  bool IsWhitelistedReferrer(const GURL& firstPartyOrigin,
                             const GURL& subresourceUrl) {
    return g_brave_browser_process->referrer_whitelist_service()->IsWhitelisted(
        firstPartyOrigin, subresourceUrl);
  }

  int GetWhitelistSize() {
    return g_brave_browser_process->referrer_whitelist_service()
        ->referrer_whitelist_.size();
  }

  void ClearWhitelist() {
    g_brave_browser_process->referrer_whitelist_service()
        ->referrer_whitelist_.clear();
  }
};

IN_PROC_BROWSER_TEST_F(ReferrerWhitelistServiceTest, IsWhitelistedReferrer) {
  ASSERT_TRUE(InstallMockExtension());
  // *.fbcdn.net not allowed on some other URL
  EXPECT_FALSE(IsWhitelistedReferrer(
      GURL("https://test.com"), GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // *.fbcdn.net allowed on Facebook
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
                                    GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // Facebook doesn't allow just anything
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
                                     GURL("https://test.com")));
  // Allowed for reddit.com
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
                                    GURL("https://www.redditmedia.com/97")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
                                    GURL("https://cdn.embedly.com/157")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
                                    GURL("https://imgur.com/179")));
  // Not allowed for reddit.com
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.reddit.com"),
                                     GURL("https://test.com")));
  // Not allowed imgur on another domain
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.test.com"),
                                     GURL("https://imgur.com/173")));
  // Fonts allowed anywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
                                    GURL("https://use.typekit.net/193")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
                                    GURL("https://cloud.typography.com/199")));
  // geetest allowed everywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://binance.com"),
                                    GURL("https://api.geetest.com/ajax.php?")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("http://binance.com"),
                                    GURL("https://api.geetest.com/")));
  // not allowed with a different scheme
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("http://binance.com"),
                                     GURL("http://api.geetest.com/")));
  // Google Accounts only allows a specific hostname
  EXPECT_TRUE(IsWhitelistedReferrer(
      GURL("https://accounts.google.com"),
      GURL("https://content.googleapis.com/cryptauth/v1/authzen/awaittx")));
  EXPECT_FALSE(IsWhitelistedReferrer(
      GURL("https://accounts.google.com"),
      GURL("https://ajax.googleapis.com/ajax/libs/d3js/5.7.0/d3.min.js")));
}

// Ensure the referrer whitelist service properly clears its cache of
// precompiled URLPatterns if initialized twice. (This can happen if
// the parent component is updated while Brave is running.)
IN_PROC_BROWSER_TEST_F(ReferrerWhitelistServiceTest, ClearCache) {
  ASSERT_TRUE(InstallMockExtension());
  int size = GetWhitelistSize();
  // clear the cache manually to make sure we're actually
  // reinitializing it the second time
  ClearWhitelist();
  ASSERT_TRUE(InstallMockExtension());
  EXPECT_EQ(size, GetWhitelistSize());
  // now reinitialize without manually clearing (simulates an in-place
  // component update)
  ASSERT_TRUE(InstallMockExtension());
  EXPECT_EQ(size, GetWhitelistSize());
}
