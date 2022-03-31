/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/cookies/cookie_monster_store_test.h"  // For CookieStore mock
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_store_unittest.h"
#include "net/log/net_log.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {

struct IPFSCookieStoreTestTraits {
  static std::unique_ptr<CookieStore> Create() {
    return std::make_unique<CookieMonster>(nullptr /* store */,
                                           nullptr /* netlog */,
                                           /*first_party_sets_enabled=*/false);
  }

  static void DeliverChangeNotifications() { base::RunLoop().RunUntilIdle(); }

  static const bool supports_http_only = true;
  static const bool supports_non_dotted_domains = true;
  static const bool preserves_trailing_dots = true;
  static const bool filters_schemes = true;
  static const bool has_path_prefix_bug = false;
  static const bool forbids_setting_empty_name = false;
  static const int creation_time_granularity_in_ms = 0;
  static const bool supports_cookie_access_semantics = true;
};

INSTANTIATE_TYPED_TEST_SUITE_P(IPFSCookieStore,
                               CookieStoreTest,
                               IPFSCookieStoreTestTraits);

class IPFSCookieStoreTest : public CookieStoreTest<IPFSCookieStoreTestTraits> {
 protected:
  GURL GetIPFSURL(const std::string& cid) {
    return GURL("http://" + cid + ".ipfs.localhost:48080");
  }

  GURL GetIPNSURL(const std::string& cid) {
    return GURL("http://" + cid + ".ipns.localhost:48080");
  }
};

TEST_F(IPFSCookieStoreTest, SetCookie) {
  scoped_refptr<MockPersistentCookieStore> store(new MockPersistentCookieStore);
  std::unique_ptr<CookieMonster> cm(new CookieMonster(
      store.get(), net::NetLog::Get(), /*first_party_sets_enabled=*/false));

  // Verify
  // 1. {CID}.ipfs.localhost can set cookies for itself
  // 2. {CID}.ipfs.localhost cannot set cookies for ipfs.localhost
  // 3. cid1.ipfs.localhost cannot access cookies set for cid2.ipfs.localhost,
  //    and vise virsa.
  GURL ipfs_url_cid1("http://cid1.ipfs.localhost:48080");
  GURL ipfs_url_cid2("http://cid2.ipfs.localhost:48080");

  EXPECT_TRUE(SetCookie(cm.get(), ipfs_url_cid1, "A=B"));
  EXPECT_TRUE(SetCookie(cm.get(), ipfs_url_cid2, "C=D"));
  EXPECT_FALSE(
      SetCookie(cm.get(), ipfs_url_cid2, "E=F; domain=ipfs.localhost"));
  MatchCookieLines("A=B", GetCookies(cm.get(), ipfs_url_cid1));
  MatchCookieLines("C=D", GetCookies(cm.get(), ipfs_url_cid2));
  MatchCookieLines("", GetCookies(cm.get(), GURL("http://ipfs.localhost")));

  // Verify above for IPNS too.
  GURL ipns_url_cid1("http://cid1.ipns.localhost:48080");
  GURL ipns_url_cid2("http://cid2.ipns.localhost:48080");

  EXPECT_TRUE(SetCookie(cm.get(), ipns_url_cid1, "G=H"));
  EXPECT_TRUE(SetCookie(cm.get(), ipns_url_cid2, "I=J"));
  EXPECT_FALSE(
      SetCookie(cm.get(), ipns_url_cid2, "L=M; domain=ipns.localhost"));
  MatchCookieLines("G=H", GetCookies(cm.get(), ipns_url_cid1));
  MatchCookieLines("I=J", GetCookies(cm.get(), ipns_url_cid2));
  MatchCookieLines("", GetCookies(cm.get(), GURL("http://ipns.localhost")));
}

}  // namespace net
