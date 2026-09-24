/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_BASE_URL_UTIL_H_
#define BRAVE_NET_BASE_URL_UTIL_H_

#include "url/gurl.h"

namespace net {

bool IsHTTPSOrLocalhostURL(const GURL& url);

}  // namespace net

#endif  // BRAVE_NET_BASE_URL_UTIL_H_
