/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_NET_DNS_BRAVE_RESOLVE_CONTEXT_H_
#define BRAVE_NET_DNS_BRAVE_RESOLVE_CONTEXT_H_

#include "net/base/net_export.h"
#include "net/dns/resolve_context.h"

namespace net {

class DnsSession;
class URLRequestContext;

class NET_EXPORT_PRIVATE BraveResolveContext : public ResolveContext {
 public:
  BraveResolveContext(URLRequestContext* url_request_context,
                      bool enable_caching);

  BraveResolveContext(const BraveResolveContext&) = delete;
  BraveResolveContext& operator=(const BraveResolveContext&) = delete;

  ~BraveResolveContext() override;

  bool GetDohServerAvailability(size_t doh_server_index,
                                const DnsSession* session) const override;
  size_t NumAvailableDohServers(const DnsSession* session) const override;

 private:
  bool IsFirstProbeCompleted(const ServerStats& stat) const;
};

}  // namespace net

#endif  // BRAVE_NET_DNS_BRAVE_RESOLVE_CONTEXT_H_
