/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/dns/host_resolver_manager.h"

#include "net/base/url_util.h"
#include "net/dns/host_resolver_manager_job.h"
#include "net/dns/resolve_context.h"
#include "net/url_request/url_request_context.h"

namespace net {
namespace {

bool IsOnionHostnameResolutionBlocked(
    const HostResolverManager::JobKey& job_key) {
  const ResolveContext* resolve_context = job_key.resolve_context.get();
  if (!resolve_context) {
    return false;
  }
  const URLRequestContext* url_request_context =
      resolve_context->url_request_context();
  return url_request_context && !url_request_context->is_tor() &&
         url_request_context->onion_only_in_tor_windows() &&
         IsOnionHostname(job_key.host.GetHostname());
}

}  // namespace
}  // namespace net

#include <net/dns/host_resolver_manager.cc>
