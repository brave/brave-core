/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/base/lookup_string_in_fixed_set.h"

#include <string_view>

#define LookupSuffixInReversedSet LookupSuffixInReversedSet_ChromiumImpl
#include "src/net/base/lookup_string_in_fixed_set.cc"
#undef LookupSuffixInReversedSet

#include "base/strings/string_util.h"
#include "brave/net/decentralized_dns/constants.h"

namespace net {

// Chromium pulls upstream public suffix list in effective_tld_names.dat and
// generate effective_tld_names.gperf from it. The list is processed by
// net/tools/dafsa/make_dafsa.py which will generate a byte array in
// effective_tld_names-reversed-inc.cc from the list to be used for comparison
// in the run-time. This function is the only function which looks up entries
// in the public suffix list, so we add our special case handling here to avoid
// patching effective_tld_names.gperf directly.
int LookupSuffixInReversedSet(base::span<const uint8_t> graph,
                              bool include_private,
                              std::string_view host,
                              size_t* suffix_length) {
  // Recognize .crypto(and other ud suffixes) and .eth as known TLDs for
  // decentralized DNS support. With this, when users type *.crypto or *.eth in
  // omnibox, it will be parsed as OmniboxInputType::URL input type instead of
  // OmniboxInputType::UNKNOWN, The first entry in the autocomplete list will be
  // URL instead of search.
  if (auto domain = decentralized_dns::GetUnstoppableDomainSuffix(host)) {
    *suffix_length = domain->size() - 1;
    return kDafsaFound;
  }
  if (host.ends_with(decentralized_dns::kEthDomain)) {
    *suffix_length = decentralized_dns::kEthDomain.size() - 1;
    return kDafsaFound;
  }
  if (host.ends_with(decentralized_dns::kSolDomain)) {
    *suffix_length = decentralized_dns::kSolDomain.size() - 1;
    return kDafsaFound;
  }

  if (include_private && host.ends_with(decentralized_dns::kDNSForEthDomain)) {
    *suffix_length = decentralized_dns::kDNSForEthDomain.size() - 1;
    return kDafsaFound;
  }

  return LookupSuffixInReversedSet_ChromiumImpl(graph, include_private, host,
                                                suffix_length);
}

}  // namespace net
