/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/canonical_cookie.h"
#include "net/cookies/parsed_cookie.h"

namespace {

const base::TimeDelta kMaxClientSideExpiration = base::TimeDelta::FromDays(7);
const base::TimeDelta kMaxServerSideExpiration =
    base::TimeDelta::FromDays(30*6);  // 6 months

base::Time BraveCanonExpiration(const net::ParsedCookie& pc,
                                const base::Time& current,
                                const base::Time& server_time,
                                const bool is_from_http) {
  const base::Time max_expiration =
      current +
      (is_from_http ? kMaxServerSideExpiration : kMaxClientSideExpiration);

  return std::min(
      net::CanonicalCookie::CanonExpiration(pc, current, server_time),
      max_expiration);
}

}  // namespace

#define BRAVE_CANONICALCOOKIE_CREATE_EXTRA_PARAMS \
  const bool is_from_http,

#define BRAVE_CANONICALCOOKIE_CREATE_BRAVECANONEXPIRATION \
  Time cookie_expires = BraveCanonExpiration(             \
      parsed_cookie, creation_time, cookie_server_time, is_from_http);

#include "../../../../net/cookies/canonical_cookie.cc"
#undef BRAVE_CANONICALCOOKIE_CREATE_BRAVECANONEXPIRATION
#undef BRAVE_CANONICALCOOKIE_CREATE_EXTRA_PARAMS
