/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <net/base/url_util_unittest.cc>

namespace net {
namespace {

TEST(UrlUtilTest, IsOnionOrigin) {
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("https://foo.onion/"))));
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("https://foo.onion:20000/"))));
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("https://foo.bar.onion/"))));
  EXPECT_FALSE(IsOnion(url::Origin::Create(GURL("https://foo.onion.com/"))));
  EXPECT_FALSE(IsOnion(url::Origin::Create(GURL("https://foo.com/b.onion"))));

  // Opaque origins (e.g. a page sandboxed via `Content-Security-Policy:
  // sandbox`) must be recognized as .onion via their precursor tuple.
  EXPECT_TRUE(IsOnion(
      url::Origin::Create(GURL("https://foo.onion/")).DeriveNewOpaqueOrigin()));
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("http://foo.bar.onion/"))
                          .DeriveNewOpaqueOrigin()));
  EXPECT_FALSE(IsOnion(
      url::Origin::Create(GURL("https://foo.com/")).DeriveNewOpaqueOrigin()));
  // A fully opaque origin with no precursor tuple is not .onion.
  EXPECT_FALSE(IsOnion(url::Origin()));
}

TEST(UrlUtilTest, IsOnionUrlSchemes) {
  EXPECT_TRUE(IsOnion(GURL("https://foo.onion/")));
  EXPECT_TRUE(IsOnion(GURL("Http://foo.bar.onion/")));
  EXPECT_TRUE(IsOnion(GURL("wSs://foo.onion/bar")));
  EXPECT_TRUE(IsOnion(GURL("WS://foo.onion/bar")));
  EXPECT_FALSE(IsOnion(GURL("file:///foo.onion/bar")));
  EXPECT_FALSE(IsOnion(GURL("ftp://foo.onion/")));
  EXPECT_FALSE(IsOnion(GURL("data:,foo.onion")));
  EXPECT_FALSE(IsOnion(GURL("chrome://onion/")));
}

TEST(UrlUtilTest, IsCrossOriginOnionRequest) {
  const auto initiator_origin = url::Origin::Create(GURL("https://foo.onion/"));

  EXPECT_TRUE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("https://bar.onion/")));
  EXPECT_TRUE(IsCrossOriginOnionRequest(initiator_origin,
                                        GURL("https://sub.foo.onion/")));
  EXPECT_TRUE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("https://foo.com/")));
  EXPECT_TRUE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("http://foo.onion/")));
  EXPECT_TRUE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("wss://bar.onion/")));
  EXPECT_TRUE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("ws://foo.onion/")));
  EXPECT_TRUE(IsCrossOriginOnionRequest(
      initiator_origin.DeriveNewOpaqueOrigin(), GURL("https://foo.onion/")));

  EXPECT_FALSE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("https://foo.onion/a")));
  // This is false because IsCrossOriginOnionRequest internally maps ws:// and
  // wss:// schemes to http:// and https:// schemes respectively and then
  // perform cross origin checks with the initiator origin. The initiator origin
  // in this case is https://foo.onion/
  EXPECT_FALSE(
      IsCrossOriginOnionRequest(initiator_origin, GURL("wss://foo.onion/a")));

  const auto foo_https_site = url::Origin::Create(GURL("https://foo.com/"));
  EXPECT_FALSE(
      IsCrossOriginOnionRequest(foo_https_site, GURL("https://bar.onion/")));
  EXPECT_FALSE(
      IsCrossOriginOnionRequest(foo_https_site, GURL("wss://bar.com/")));
  EXPECT_FALSE(IsCrossOriginOnionRequest(url::Origin(), GURL("https://a.com")));
}

}  // namespace
}  // namespace net
