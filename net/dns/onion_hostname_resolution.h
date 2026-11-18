/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_DNS_ONION_HOSTNAME_RESOLUTION_H_
#define BRAVE_NET_DNS_ONION_HOSTNAME_RESOLUTION_H_

#include <string_view>

namespace net {

class ResolveContext;

// Returns whether a lookup of `hostname` for `resolve_context` must not reach
// the system resolver or DoH: `hostname` is .onion and the context's
// URLRequestContext::TorMode is kDisabled. Always false in builds without Tor.
bool IsOnionHostnameResolutionBlocked(const ResolveContext* resolve_context,
                                      std::string_view hostname);

}  // namespace net

#endif  // BRAVE_NET_DNS_ONION_HOSTNAME_RESOLUTION_H_
