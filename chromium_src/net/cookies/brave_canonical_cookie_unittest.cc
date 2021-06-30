/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/canonical_cookie.h"

#include "base/time/time.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_options.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::TimeDelta;

static const char max_expires_date_cookie[] =
    "test1=yes; expires=Fri, 31 Dec 9999 23:59:59 GMT";
static const char max_age_cookie[] =
    "test2=yes; max-age=630720000";  // 20 years
static const char max_expires_and_max_age_cookie[] =
    "test3=yes; max-age=630720000; expires=Fri, 31 Dec 9999 23:59:59 GMT";
static const char short_expiration_cookie[] =
    "test4=yes; max-age=172800";  // 2 days
static const char cookie_line5[] =
    "test5=yes; httponly; expires=Fri, 31 Dec 9999 23:59:59 GMT";
static const char no_expiration_cookie[] =
    "test5=yes";

namespace net {

const base::Time creation_time = base::Time::Now();
constexpr base::TimeDelta kMaxCookieExpiration =
    base::TimeDelta::FromDays(30*6);  // 6 months

TEST(BraveCanonicalCookieTest, SetMaxExpiration) {
  GURL url("https://www.example.com/test");

  std::unique_ptr<CanonicalCookie> cookie(CanonicalCookie::Create(
      url, max_expires_date_cookie, creation_time, absl::nullopt));
  EXPECT_TRUE(cookie.get());
  EXPECT_EQ(cookie->ExpiryDate(), creation_time + kMaxCookieExpiration);

  cookie = CanonicalCookie::Create(url, max_age_cookie, creation_time,
                                   absl::nullopt);
  EXPECT_TRUE(cookie.get());
  EXPECT_EQ(cookie->ExpiryDate(), creation_time + kMaxCookieExpiration);

  cookie = CanonicalCookie::Create(url, max_expires_and_max_age_cookie,
                                   creation_time, absl::nullopt);
  EXPECT_TRUE(cookie.get());
  EXPECT_EQ(cookie->ExpiryDate(), creation_time + kMaxCookieExpiration);
}

TEST(BraveCanonicalCookieTest, AllowShorterThanMaxExpiration) {
  GURL url("https://www.example.com/test");
  // Short-lived cookies get to keep their shorter expiration.
  std::unique_ptr<CanonicalCookie> cookie = CanonicalCookie::Create(
      url, short_expiration_cookie, creation_time, absl::nullopt);
  EXPECT_TRUE(cookie.get());
  EXPECT_EQ(cookie->ExpiryDate(), creation_time + TimeDelta::FromDays(2));
}

TEST(BraveCanonicalCookieTest, SetHTTPOnlyMaxExpiration) {
  GURL url("https://www.example.com/test");

  // HTTP cookies with 'httponly' work as expected.
  std::unique_ptr<CanonicalCookie> cookie =
      CanonicalCookie::Create(url, cookie_line5, creation_time, absl::nullopt);
  EXPECT_TRUE(cookie.get());
  EXPECT_EQ(cookie->ExpiryDate(), creation_time + kMaxCookieExpiration);
}

TEST(BraveCanonicalCookieTest, NoExpirationCookie) {
  GURL url("https://www.example.com/test");

  base::Time creation_time = base::Time::Now();

  std::unique_ptr<CanonicalCookie> cookie(CanonicalCookie::Create(
      url, no_expiration_cookie, creation_time, absl::nullopt));
  EXPECT_TRUE(cookie.get());
  EXPECT_EQ(cookie->IsPersistent(), false);
}

}  // namespace net
