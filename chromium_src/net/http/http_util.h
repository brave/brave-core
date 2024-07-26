/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_HTTP_HTTP_UTIL_H_
#define BRAVE_CHROMIUM_SRC_NET_HTTP_HTTP_UTIL_H_

#define IsNonCoalescingHeader                   \
  IsNonCoalescingHeader(std::string_view name); \
  static bool IsNonCoalescingHeader_ChromiumImpl

#include "src/net/http/http_util.h"  // IWYU pragma: export

#undef IsNonCoalescingHeader

#endif  // BRAVE_CHROMIUM_SRC_NET_HTTP_HTTP_UTIL_H_
