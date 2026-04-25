/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_STP_UTIL_H_
#define BRAVE_BROWSER_NET_BRAVE_STP_UTIL_H_

#include <array>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave {

inline constexpr auto kTrackableSecurityHeaders =
    std::to_array<std::string_view>({"Strict-Transport-Security", "Expect-CT",
                                     "Public-Key-Pins",
                                     "Public-Key-Pins-Report-Only"});

void RemoveTrackableSecurityHeadersForThirdParty(
    const GURL& request_url, const url::Origin& top_frame_origin,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_STP_UTIL_H_
