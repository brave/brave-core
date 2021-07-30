/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_H_
#define BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_H_

#include "net/base/isolation_info.h"
#include "net/base/request_priority.h"
#include "net/cookies/cookie_options.h"

#define NotifyBeforeStartTransactionCallback                        \
  NotUsed() const {}                                                \
  CookieOptions CreateCookieOptions(                                \
      CookieOptions::SameSiteCookieContext same_site_context,       \
      CookieOptions::SamePartyCookieContextType same_party_context, \
      const IsolationInfo& isolation_info,                          \
      bool is_in_nontrivial_first_party_set) const;                 \
  void NotifyBeforeStartTransactionCallback

#include "../../../../net/url_request/url_request_http_job.h"

#undef NotifyBeforeStartTransactionCallback

#endif  // BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_HTTP_JOB_H_
