/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_REPORT_SYNC_STOPPED                    \
  std::string headers;                               \
  headers = "Authorization: Bearer " + access_token; \
  resource_request->headers.AddHeadersFromString(headers.c_str());

#include "../../../../../components/sync/driver/sync_stopped_reporter.cc"

#undef BRAVE_REPORT_SYNC_STOPPED
