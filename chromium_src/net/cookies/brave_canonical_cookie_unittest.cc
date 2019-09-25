/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/canonical_cookie.h"

#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_options.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

static const std::string cookie_line1 =
    "test1=yes; expires=Fri, 31 Dec 9999 23:59:59 GMT";
static const std::string cookie_line2 =
    "test2=yes; max-age=630720000";  // 20 years
static const std::string cookie_line3 =
    "test3=yes; max-age=630720000; expires=Fri, 31 Dec 9999 23:59:59 GMT";
static const std::string cookie_line4 =
    "test4=yes; max-age=172800";  // 2 days
static const std::string cookie_line5 =
    "test5=yes; httponly; expires=Fri, 31 Dec 9999 23:59:59 GMT";

namespace net {

TEST(BraveCanonicalCookieTest, ClientSide) {
  using base::TimeDelta;

  GURL url("https://www.example.com/test");
  base::Time creation_time = base::Time::Now();
  bool is_from_http = false;

  std::unique_ptr<CanonicalCookie> cookie(
      CanonicalCookie::Create(is_from_http, url, cookie_line1, creation_time,
                              base::nullopt /* server_time */));
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));

  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line2, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));

  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line3, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));

  // Short-lived cookies get to keep their shorter expiration.
  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line4, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(3));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(1));

  // document.cookie API 'httponly' works as expected.
  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line5, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));
}

TEST(BraveCanonicalCookieTest, ServerSide) {
  using base::TimeDelta;

  GURL url("https://www.example.com/test");
  base::Time creation_time = base::Time::Now();
  bool is_from_http = true;

  std::unique_ptr<CanonicalCookie> cookie(
      CanonicalCookie::Create(is_from_http, url, cookie_line1, creation_time,
                              base::nullopt /* server_time */));
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*5));

  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line2, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*5));

  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line3, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*5));

  // Short-lived cookies get to keep their shorter expiration.
  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line4, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(3));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(1));

  // HTTP cookies with 'httponly' work as expected.
  cookie =
      CanonicalCookie::Create(is_from_http, url, cookie_line5, creation_time,
                              base::nullopt /* server_time */);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30*5));
}

}  // namespace
