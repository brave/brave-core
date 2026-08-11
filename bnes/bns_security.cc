// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/bnes/bns_security.h"

#include <cctype>
#include <string>

#include "base/strings/string_util.h"
#include "net/base/ip_address.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace bnes {

namespace {

constexpr std::string_view kBnesSuffix = ".bnes";
constexpr size_t kMinCidLength = 4;
constexpr size_t kMaxCidLength = 256;

// DNS label rules per RFC 952 / RFC 1123: alphanumeric plus hyphens, no
// leading/trailing hyphen, max 63 characters.
bool IsValidDnsLabel(std::string_view label) {
  if (label.empty() || label.size() > 63 || label.front() == '-' ||
      label.back() == '-') {
    return false;
  }
  for (const char character : label) {
    if (!base::IsAsciiAlphaNumeric(character) && character != '-') {
      return false;
    }
  }
  return true;
}

bool HasOnlyValidDnsLabels(std::string_view host) {
  size_t label_start = 0;
  while (label_start < host.size()) {
    const size_t label_end = host.find('.', label_start);
    const size_t label_length = (label_end == std::string_view::npos)
                                    ? host.size() - label_start
                                    : label_end - label_start;
    if (!IsValidDnsLabel(host.substr(label_start, label_length))) {
      return false;
    }
    if (label_end == std::string_view::npos) {
      return true;
    }
    label_start = label_end + 1;
  }
  return false;
}

bool HasForbiddenUrlParts(const GURL& url) {
  return url.has_username() || url.has_password() || url.has_port() ||
         url.has_query() || url.has_ref();
}

bool HasForbiddenAuthorityParts(const GURL& url) {
  return url.has_username() || url.has_password() || url.has_port();
}

constexpr bool IsBase58Character(char character) {
  return base::IsAsciiAlphaNumeric(character) && character != '0' &&
         character != 'O' && character != 'I' && character != 'l';
}

constexpr bool IsBase32LowerCharacter(char character) {
  return (character >= 'a' && character <= 'z') ||
         (character >= '2' && character <= '7');
}

constexpr bool HasBnesSuffix(std::string_view host) {
  return host.size() > kBnesSuffix.size() &&
         base::EndsWith(host, kBnesSuffix, base::CompareCase::SENSITIVE);
}

}  // namespace

bool IsValidBnesHost(std::string_view host) {
  if (host.empty() || host.size() <= kBnesSuffix.size()) {
    return false;
  }

  std::string lowered;
  lowered.reserve(host.size());
  for (const unsigned char c : host) {
    lowered.push_back(std::tolower(c));
  }

  return HasOnlyValidDnsLabels(lowered) && HasBnesSuffix(lowered);
}

bool IsAllowedNavigationUrl(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs("bnes") ||
      HasForbiddenAuthorityParts(url) ||
      url.HostIsIPAddress()) {
    return false;
  }

  const std::string host = base::ToLowerASCII(url.host());
  return HasBnesSuffix(host) && HasOnlyValidDnsLabels(host);
}

bool IsAllowedGatewayUrl(const GURL& url,
                         std::string_view trusted_gateway_host) {
  return url.is_valid() && url.SchemeIs(url::kHttpsScheme) &&
         !HasForbiddenUrlParts(url) && !url.HostIsIPAddress() &&
         base::EqualsCaseInsensitiveASCII(url.host(), trusted_gateway_host);
}

bool IsPublicNetworkAddress(const net::IPAddress& address) {
  // IsPubliclyRoutable covers private IPv4, loopback, link-local, multicast,
  // documentation and unique-local ranges. Keep explicit checks to make the
  // invariant robust when callers pass IPv6-mapped or future address types.
  return address.IsValid() && address.IsPubliclyRoutable() &&
         !address.IsLoopback() && !address.IsLinkLocal() &&
         !address.IsUniqueLocalIPv6();
}

bool IsValidCid(std::string_view cid) {
  if (cid.size() < kMinCidLength || cid.size() > kMaxCidLength) {
    return false;
  }

  // CIDv0 is always a 46-character base58btc multihash beginning with Qm.
  if (cid.size() >= 2 && cid[0] == 'Q' && cid[1] == 'm') {
    if (cid.size() != 46) {
      return false;
    }
    for (const char character : cid) {
      if (!IsBase58Character(character)) {
        return false;
      }
    }
    return true;
  }

  // Use only the canonical lower-case base32 representation for CIDv1. It is
  // safe to interpolate into /ipfs/<cid> and avoids accepting a lookalike or
  // alternate-base representation that the gateway may normalize differently.
  if (cid.empty() || cid[0] != 'b') {
    return false;
  }
  for (const char character : cid) {
    if (!IsBase32LowerCharacter(character)) {
      return false;
    }
  }
  return true;
}

}  // namespace bnes
