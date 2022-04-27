/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/unstoppable_domains_dns_resolve.h"

#include "base/no_destructor.h"

namespace brave_wallet::unstoppable_domains {

namespace {
enum RecordKeys {
  DWEB_IPFS_HASH,
  IPFS_HTML_VALUE,
  DNS_A,
  DNS_AAAA,
  BROWSER_REDIRECT_URL,
  IPFS_REDIRECT_DOMAIN_VALUE,
  RECORD_KEY_COUNT,
};

// See
// https://docs.unstoppabledomains.com/developer-toolkit/resolve-domains-browser/browser-resolution-algorithm/
// for more details.
const constexpr char* const kRecordKeys[] = {
    "dweb.ipfs.hash", "ipfs.html.value",      "dns.A",
    "dns.AAAA",       "browser.redirect_url", "ipfs.redirect_domain.value"};
static_assert(static_cast<size_t>(RecordKeys::RECORD_KEY_COUNT) ==
                  sizeof(kRecordKeys) / sizeof(kRecordKeys[0]),
              "Size should match between RecordKeys and kRecordKeys.");
}  // namespace

GURL ResolveUrl(const std::vector<std::string>& response) {
  if (response.size() != GetRecordKeys().size())
    return GURL();

  // TODO(jocelyn): Do not fallback to the set redirect URL if dns.A or
  // dns.AAAA is not empty once we support the classical DNS records case.
  std::string ipfs_uri = response[RecordKeys::DWEB_IPFS_HASH];
  if (ipfs_uri.empty()) {  // Try legacy value.
    ipfs_uri = response[RecordKeys::IPFS_HTML_VALUE];
  }
  if (!ipfs_uri.empty()) {
    return GURL("ipfs://" + ipfs_uri);
  }

  std::string fallback_url = response[RecordKeys::BROWSER_REDIRECT_URL];
  if (fallback_url.empty()) {  // Try legacy value.
    fallback_url = response[RecordKeys::IPFS_REDIRECT_DOMAIN_VALUE];
  }
  if (!fallback_url.empty()) {
    return GURL(fallback_url);
  }

  return GURL();
}

const std::vector<std::string>& GetRecordKeys() {
  static base::NoDestructor<std::vector<std::string>> keys(
      std::begin(kRecordKeys), std::end(kRecordKeys));
  return *keys;
}
}  // namespace brave_wallet::unstoppable_domains
