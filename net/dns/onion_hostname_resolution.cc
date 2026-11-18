/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/dns/onion_hostname_resolution.h"

#include "brave/components/tor/buildflags/buildflags.h"
#include "net/base/url_util.h"
#include "net/dns/resolve_context.h"
#include "net/url_request/url_request_context.h"

namespace net {

bool IsOnionHostnameResolutionBlocked(const ResolveContext* resolve_context,
                                      std::string_view hostname) {
#if BUILDFLAG(ENABLE_TOR)
  if (!resolve_context) {
    return false;
  }
  const URLRequestContext* url_request_context =
      resolve_context->url_request_context();
  return url_request_context &&
         url_request_context->tor_mode() ==
             URLRequestContext::TorMode::kDisabled &&
         IsOnionHostname(hostname);
#else
  return false;
#endif  // BUILDFLAG(ENABLE_TOR)
}

}  // namespace net
