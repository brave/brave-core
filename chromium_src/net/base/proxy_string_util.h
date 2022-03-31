/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_PROXY_STRING_UTIL_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_PROXY_STRING_UTIL_H_

#define ProxyServerToProxyUri                                          \
  ProxyServerToProxyUri_ChromiumImpl(const ProxyServer& proxy_server); \
  NET_EXPORT std::string ProxyServerToProxyUri

#include "src/net/base/proxy_string_util.h"

#undef ProxyServerToProxyUri

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_PROXY_STRING_UTIL_H_
