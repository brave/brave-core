/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/webcat_cache.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace webcat {

namespace {

Bundle MakeBundle(const std::string& app) {
  Bundle b;
  b.manifest.app = app;
  b.manifest.version = "1.0.0";
  b.manifest.default_csp = "default-src 'self'";
  b.manifest.default_index = "/index.html";
  b.manifest.default_fallback = "/index.html";
  b.manifest.files["/index.html"] = "hash1";
  return b;
}

}  // namespace

TEST(WebcatCacheTest, PutAndGet) {
  WebcatCache cache(10, base::Hours(1));
  auto origin = url::Origin::Create(GURL("https://app.eth"));

  Bundle bundle = MakeBundle("https://app.eth");
  cache.Put(origin, std::move(bundle), "cid1");

  auto cached = cache.Get(origin);
  ASSERT_TRUE(cached.has_value());
  EXPECT_EQ(cached->cid, "cid1");
  EXPECT_EQ(cached->bundle.manifest.app, "https://app.eth");
}

TEST(WebcatCacheTest, GetNonExistentReturnsNullopt) {
  WebcatCache cache(10, base::Hours(1));
  auto origin = url::Origin::Create(GURL("https://nonexistent.eth"));

  auto cached = cache.Get(origin);
  EXPECT_FALSE(cached.has_value());
}

TEST(WebcatCacheTest, Remove) {
  WebcatCache cache(10, base::Hours(1));
  auto origin = url::Origin::Create(GURL("https://app.eth"));

  cache.Put(origin, MakeBundle("https://app.eth"), "cid1");
  EXPECT_TRUE(cache.Contains(origin));

  cache.Remove(origin);
  EXPECT_FALSE(cache.Contains(origin));
  EXPECT_FALSE(cache.Get(origin).has_value());
}

TEST(WebcatCacheTest, Clear) {
  WebcatCache cache(10, base::Hours(1));
  auto origin1 = url::Origin::Create(GURL("https://app1.eth"));
  auto origin2 = url::Origin::Create(GURL("https://app2.eth"));

  cache.Put(origin1, MakeBundle("https://app1.eth"), "cid1");
  cache.Put(origin2, MakeBundle("https://app2.eth"), "cid2");
  EXPECT_EQ(cache.size(), 2u);

  cache.Clear();
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_FALSE(cache.Get(origin1).has_value());
  EXPECT_FALSE(cache.Get(origin2).has_value());
}

TEST(WebcatCacheTest, EvictsOldestWhenFull) {
  WebcatCache cache(2, base::Hours(1));
  auto origin1 = url::Origin::Create(GURL("https://app1.eth"));
  auto origin2 = url::Origin::Create(GURL("https://app2.eth"));
  auto origin3 = url::Origin::Create(GURL("https://app3.eth"));

  cache.Put(origin1, MakeBundle("https://app1.eth"), "cid1");
  cache.Put(origin2, MakeBundle("https://app2.eth"), "cid2");
  EXPECT_EQ(cache.size(), 2u);

  cache.Put(origin3, MakeBundle("https://app3.eth"), "cid3");
  EXPECT_EQ(cache.size(), 2u);

  EXPECT_FALSE(cache.Contains(origin1));
  EXPECT_TRUE(cache.Contains(origin2));
  EXPECT_TRUE(cache.Contains(origin3));
}

TEST(WebcatCacheTest, UpdateCidRemovesOnMismatch) {
  WebcatCache cache(10, base::Hours(1));
  auto origin = url::Origin::Create(GURL("https://app.eth"));

  cache.Put(origin, MakeBundle("https://app.eth"), "cid1");
  EXPECT_TRUE(cache.Contains(origin));

  cache.UpdateCid(origin, "cid2");
  EXPECT_FALSE(cache.Contains(origin));
  EXPECT_FALSE(cache.Get(origin).has_value());
}

TEST(WebcatCacheTest, UpdateCidKeepsOnMatch) {
  WebcatCache cache(10, base::Hours(1));
  auto origin = url::Origin::Create(GURL("https://app.eth"));

  cache.Put(origin, MakeBundle("https://app.eth"), "cid1");
  EXPECT_TRUE(cache.Contains(origin));

  cache.UpdateCid(origin, "cid1");
  EXPECT_TRUE(cache.Contains(origin));
  auto cached = cache.Get(origin);
  ASSERT_TRUE(cached.has_value());
  EXPECT_EQ(cached->cid, "cid1");
}

TEST(WebcatCacheTest, LruAccessOrder) {
  WebcatCache cache(2, base::Hours(1));
  auto origin1 = url::Origin::Create(GURL("https://app1.eth"));
  auto origin2 = url::Origin::Create(GURL("https://app2.eth"));
  auto origin3 = url::Origin::Create(GURL("https://app3.eth"));

  cache.Put(origin1, MakeBundle("https://app1.eth"), "cid1");
  cache.Put(origin2, MakeBundle("https://app2.eth"), "cid2");

  cache.Get(origin1);

  cache.Put(origin3, MakeBundle("https://app3.eth"), "cid3");

  EXPECT_TRUE(cache.Contains(origin1));
  EXPECT_FALSE(cache.Contains(origin2));
  EXPECT_TRUE(cache.Contains(origin3));
}

}  // namespace webcat