/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_H_
#define BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_H_

#define ShouldFixMismatchedContentLength                                      \
  CanGetNonEphemeralCookies();                                                \
  bool CanSetNonEphemeralCookie(const net::CanonicalCookie&, CookieOptions*); \
  bool CanSetCookieIncludingEphemeral(const net::CanonicalCookie&,            \
                                      CookieOptions*);                        \
  bool ShouldFixMismatchedContentLength

#include "../../../../net/url_request/url_request_http_job.h"

#undef ShouldFixMismatchedContentLength

#endif  // BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_H_
