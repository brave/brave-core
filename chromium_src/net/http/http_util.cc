/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/http_util.h"

#define IsNonCoalescingHeader IsNonCoalescingHeader_ChromiumImpl
#include "src/net/http/http_util.cc"
#undef IsNonCoalescingHeader

namespace net {

// static
bool HttpUtil::IsNonCoalescingHeader(std::string_view name) {
  if (base::EqualsCaseInsensitiveASCII(name, "onion-location")) {
    return true;
  }
  return IsNonCoalescingHeader_ChromiumImpl(name);
}

}  // namespace net
