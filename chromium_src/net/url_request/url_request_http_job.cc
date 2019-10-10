/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_SAVECOOKIESANDNOTIFYHEADERSCOMPLETE \
  !options.exclude_httponly() /*is_from_http*/,

#include "../../../../net/url_request/url_request_http_job.cc"
#undef BRAVE_SAVECOOKIESANDNOTIFYHEADERSCOMPLETE
