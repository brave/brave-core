/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONSTANTS_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONSTANTS_H_

namespace decentralized_dns {

enum class Provider {
  UNSTOPPABLE_DOMAINS,
  ENS,
};

// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.decentralized_dns
enum class ResolveMethodTypes {
  ASK,
  DISABLED,
  DNS_OVER_HTTPS,
  ETHEREUM,
  MAX_VALUE = ETHEREUM,
};

enum class RecordKeys {
  DWEB_IPFS_HASH,
  IPFS_HTML_VALUE,
  DNS_A,
  DNS_AAAA,
  BROWSER_REDIRECT_URL,
  IPFS_REDIRECT_DOMAIN_VALUE,
  MAX_RECORD_KEY = IPFS_REDIRECT_DOMAIN_VALUE,
};

// Need to match RecordKeys above. See
// https://docs.unstoppabledomains.com/browser-resolution/browser-resolution-algorithm#browser-resolution-records
// for more details.
const constexpr char* const kRecordKeys[] = {
    "dweb.ipfs.hash", "ipfs.html.value",      "dns.A",
    "dns.AAAA",       "browser.redirect_url", "ipfs.redirect_domain.value"};

static_assert(static_cast<size_t>(RecordKeys::MAX_RECORD_KEY) + 1u ==
                  sizeof(kRecordKeys) / sizeof(kRecordKeys[0]),
              "Size should match between RecordKeys and kRecordKeys.");

constexpr char kProxyReaderContractAddress[] =
    "0xa6E7cEf2EDDEA66352Fd68E5915b60BDbb7309f5";

constexpr char kEnsRegistryContractAddress[] =
    "0x4976fb03C32e5B8cfe2b6cCB31c09Ba78EBaBa41";

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONSTANTS_H_
