/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/time/time.h"

namespace {

constexpr base::TimeDelta kMaxCookieExpiration =
    base::Days(30 * 6);  // 6 months

base::Time BraveCanonExpiration(const base::Time& expiry_date,
                                const base::Time& creation_date) {
  const base::Time max_expiration = creation_date + kMaxCookieExpiration;

  return std::min(expiry_date, max_expiration);
}

}  // namespace

#define BRAVE_CANONICAL_COOKIE_VALIDATE_AND_ADJUST_EXPIRY_DATE \
  if ((true))                                                  \
    return BraveCanonExpiration(expiry_date, fixed_creation_date);

#include "src/net/cookies/canonical_cookie.cc"
#undef BRAVE_CANONICAL_COOKIE_VALIDATE_AND_ADJUST_EXPIRY_DATE
