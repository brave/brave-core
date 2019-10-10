/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_COOKIES_CANONICAL_COOKIE_H_
#define BRAVE_CHROMIUM_SRC_NET_COOKIES_CANONICAL_COOKIE_H_

#define BRAVE_CANONICAL_COOKIE_H_CREATE_EXTRA_PARAMS \
  const bool is_from_http,

#include "../../../../net/cookies/canonical_cookie.h"
#undef BRAVE_CANONICAL_COOKIE_H_CREATE_EXTRA_PARAMS

#endif  // BRAVE_CHROMIUM_SRC_NET_COOKIES_CANONICAL_COOKIE_H_
