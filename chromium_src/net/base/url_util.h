// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_URL_UTIL_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_URL_UTIL_H_

#include <net/base/url_util.h>  // IWYU pragma: export

#include "net/base/net_export.h"
#include "url/origin.h"

namespace net {

// Convert this URL into an encoded storage domain string, which is used
// to identify a particular storage domain uniquely in a BrowserContext.
NET_EXPORT std::string URLToEphemeralStorageDomain(const GURL& url);

NET_EXPORT bool IsOnion(const GURL& url);

NET_EXPORT bool IsOnion(const url::Origin& origin);

NET_EXPORT bool IsLocalhostOrOnion(const GURL& url);

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_URL_UTIL_H_
