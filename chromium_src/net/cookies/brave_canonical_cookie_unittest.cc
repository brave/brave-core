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
static const std::string cookie_line4 = "test4=yes; max-age=172800";  // 2 days
static const std::string cookie_line5 =
    "test5=yes; httponly; expires=Fri, 31 Dec 9999 23:59:59 GMT";

namespace net {

TEST(BraveCanonicalCookieTest, ClientSide) {
  using base::TimeDelta;

  GURL url("https://www.example.com/test");
  base::Time creation_time = base::Time::Now();
  CookieOptions options;

  std::unique_ptr<CanonicalCookie> cookie(
      CanonicalCookie::Create(url, cookie_line1, creation_time, options));
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));

  cookie = CanonicalCookie::Create(url, cookie_line2, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));

  cookie = CanonicalCookie::Create(url, cookie_line3, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(8));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(6));

  // Short-lived cookies get to keep their shorter expiration.
  cookie = CanonicalCookie::Create(url, cookie_line4, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(3));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(1));

  // Cookies with 'httponly' can't be set using the document.cookie API.
  cookie = CanonicalCookie::Create(url, cookie_line5, creation_time, options);
  EXPECT_FALSE(cookie.get());
}

TEST(BraveCanonicalCookieTest, ServerSide) {
  using base::TimeDelta;

  GURL url("https://www.example.com/test");
  base::Time creation_time = base::Time::Now();
  CookieOptions options;
  options.set_include_httponly();

  std::unique_ptr<CanonicalCookie> cookie(
      CanonicalCookie::Create(url, cookie_line1, creation_time, options));
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 5));

  cookie = CanonicalCookie::Create(url, cookie_line2, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 5));

  cookie = CanonicalCookie::Create(url, cookie_line3, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 5));

  // Short-lived cookies get to keep their shorter expiration.
  cookie = CanonicalCookie::Create(url, cookie_line4, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(3));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(1));

  // HTTP cookies with 'httponly' work as expected.
  cookie = CanonicalCookie::Create(url, cookie_line5, creation_time, options);
  EXPECT_TRUE(cookie.get());
  EXPECT_LT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 7));
  EXPECT_GT(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(30 * 5));
}

}  // namespace net
