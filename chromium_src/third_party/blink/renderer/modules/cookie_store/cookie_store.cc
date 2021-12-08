/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/time/time.h"

namespace {

constexpr base::TimeDelta kJavascriptCookieExpiration = base::Days(7);

base::Time BraveCanonExpiration(const base::Time& current) {
  // creation time is always now for new JS cookies
  return std::min(base::Time::Now() + kJavascriptCookieExpiration, current);
}

}  // namespace

#define BRAVE_TO_CANONICAL_COOKIE expires = BraveCanonExpiration(expires);
#include "../../../../../../../third_party/blink/renderer/modules/cookie_store/cookie_store.cc"
#undef BRAVE_TO_CANONICAL_COOKIE
