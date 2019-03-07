/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_shields/browser/https_everywhere_recently_used_cache.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(HTTPSEverywhereRecentlyUsedCacheTest, Operations) {
  using Cache = HTTPSERecentlyUsedCache<std::string>;
  Cache cache(3);

  // Test add/get and check that max size is maintained.
  cache.add("kA", "vA");
  cache.add("kB", "vB");
  cache.add("kC", "vC");
  std::string v;
  ASSERT_TRUE(cache.get("kA", &v));
  ASSERT_STREQ(v.c_str(), "vA");
  // kA just became MRU, so adding a new k/v pair should evict the oldest.
  cache.add("kD", "vD");
  ASSERT_FALSE(cache.get("kB", &v));
  ASSERT_TRUE(cache.get("kD", &v));

  // Test remove.
  cache.remove("kD");
  ASSERT_FALSE(cache.get("kD", &v));
}
