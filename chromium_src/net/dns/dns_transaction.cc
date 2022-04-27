/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/strings/string_util.h"
#include "brave/net/decentralized_dns/constants.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_server_iterator.h"

namespace {

bool GetNextIndex(const std::string& hostname,
                  const net::DnsConfig& config,
                  net::DnsServerIterator* dns_server_iterator,
                  size_t* doh_server_index) {
  base::StringPiece server =
      config.doh_config.servers()[*doh_server_index].server_template();

  bool is_unstoppable_domain = std::any_of(
      std::begin(decentralized_dns::kUnstoppableDomains),
      std::end(decentralized_dns::kUnstoppableDomains),
      [&hostname](auto* domain) { return base::EndsWith(hostname, domain); });
  bool is_eth_domain = base::EndsWith(hostname, decentralized_dns::kEthDomain);

  // Skip decentralized DNS resolvers if it is not target TLDs.
  while ((server == decentralized_dns::kUnstoppableDomainsDoHResolver &&
          !is_unstoppable_domain) ||
         (server == decentralized_dns::kENSDoHResolver && !is_eth_domain)) {
    // No next available index to attempt.
    if (!dns_server_iterator->AttemptAvailable()) {
      return false;
    }

    *doh_server_index = dns_server_iterator->GetNextAttemptIndex();
    server = config.doh_config.servers()[*doh_server_index].server_template();
  }

  return true;
}

}  // namespace

#define BRAVE_MAKE_HTTP_ATTEMPT                                       \
  if (!GetNextIndex(hostname_, session_.get()->config(),              \
                    dns_server_iterator_.get(), &doh_server_index)) { \
    return AttemptResult(ERR_BLOCKED_BY_CLIENT, nullptr);             \
  }

#include "src/net/dns/dns_transaction.cc"
#undef BRAVE_MAKE_HTTP_ATTEMPT
