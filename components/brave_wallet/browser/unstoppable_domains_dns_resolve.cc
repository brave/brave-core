/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/unstoppable_domains_dns_resolve.h"

#include "base/containers/span.h"
#include "brave/components/ipfs/ipfs_utils.h"

namespace brave_wallet::unstoppable_domains {

namespace {
enum RecordKeys {
  kDwebIpfsHash,
  kIpfsHtmlValue,
  kDnsA,
  kDnsAAAA,
  kBrowserRedirectUrl,
  kIpfsRedirectValue,
  kKeyCount,
};

static_assert(std::size(kRecordKeys) ==
                  static_cast<size_t>(RecordKeys::kKeyCount),
              "Size should match between RecordKeys and kRecordKeys.");
}  // namespace

GURL ResolveUrl(base::span<const std::string> response) {
  if (response.size() != kRecordKeys.size()) {
    return GURL();
  }

  // TODO(jocelyn): Do not fallback to the set redirect URL if dns.A or
  // dns.AAAA is not empty once we support the classical DNS records case.
  std::string ipfs_uri = response[RecordKeys::kDwebIpfsHash];
  if (ipfs_uri.empty()) {  // Try legacy value.
    ipfs_uri = response[RecordKeys::kIpfsHtmlValue];
  }
  GURL resolved_url;
  if (!ipfs_uri.empty()) {
    return ipfs::TranslateIPFSURI(GURL("ipfs://" + ipfs_uri), &resolved_url,
                                  false)
               ? resolved_url
               : GURL();
  }

  std::string fallback_url = response[RecordKeys::kBrowserRedirectUrl];
  if (fallback_url.empty()) {  // Try legacy value.
    fallback_url = response[RecordKeys::kIpfsRedirectValue];
  }
  if (!fallback_url.empty()) {
    return GURL(fallback_url);
  }

  return GURL();
}

}  // namespace brave_wallet::unstoppable_domains
