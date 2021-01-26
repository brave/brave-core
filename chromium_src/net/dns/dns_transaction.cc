/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "brave/components/unstoppable_domains/buildflags/buildflags.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_server_iterator.h"

#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
#include "brave/components/unstoppable_domains/constants.h"
#endif

namespace {

bool GetNextIndex(const std::string& hostname,
                  const net::DnsConfig& config,
                  net::DnsServerIterator* dns_server_iterator,
                  size_t* doh_server_index) {
#if BUILDFLAG(UNSTOPPABLE_DOMAINS_ENABLED)
  // Skip unstoppable domains resolver for non-crypto domains.
  if (!base::EndsWith(hostname, unstoppable_domains::kCryptoDomain) &&
      config.dns_over_https_servers[*doh_server_index].server_template ==
          unstoppable_domains::kDoHResolver) {
    // No next available index to attempt.
    if (!dns_server_iterator->AttemptAvailable()) {
      return false;
    }

    *doh_server_index = dns_server_iterator->GetNextAttemptIndex();
  }
#endif

  return true;
}

}  // namespace

#define BRAVE_MAKE_HTTP_ATTEMPT                                       \
  if (!GetNextIndex(hostname_, session_.get()->config(),              \
                    dns_server_iterator_.get(), &doh_server_index)) { \
    return AttemptResult(ERR_BLOCKED_BY_CLIENT, nullptr);             \
  }

#include "../../../../net/dns/dns_transaction.cc"
#undef BRAVE_MAKE_HTTP_ATTEMPT
