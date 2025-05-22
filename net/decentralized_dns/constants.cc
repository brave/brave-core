/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/decentralized_dns/constants.h"

#include <algorithm>

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_util.h"

namespace decentralized_dns {

namespace {

// A struct to be used to guide the lookup of domain suffixes in the fixed flat
// set below. Lookups using this wrapper will match based on the suffix after
// the last '.' in the domain. Lookups for suffix can be done like:
//
// kUnstoppableDomains.contains(DomainSuffixLookup("https://foo.crypto"))
//
struct DomainSuffixLookup {
  constexpr explicit DomainSuffixLookup(std::string_view domain)
      : domain(ExtractSuffix(domain)) {}

  // Used to initialise the view from the last '.' in the domain, if any exist.
  // Otherwise, passes the whole string for comparison.
  static std::string_view ExtractSuffix(std::string_view domain) {
    size_t pos = domain.find_last_of('.');
    return pos != std::string_view::npos ? domain.substr(pos) : domain;
  }

  std::string_view domain;
};

// A custom comparator with strict weak ordering, to allow the lookup of a
// domain with `DomainSuffixLookup`.
struct SuffixComparator {
  using is_transparent = void;

  constexpr bool operator()(const std::string_view& lhs,
                            const std::string_view& rhs) const {
    return lhs < rhs;
  }

  constexpr bool operator()(const std::string_view& lhs,
                            DomainSuffixLookup rhs) const {
    return (*this)(lhs, rhs.domain);
  }

  constexpr bool operator()(DomainSuffixLookup lhs,
                            const std::string_view& rhs) const {
    return (*this)(lhs.domain, rhs);
  }
};

inline constexpr auto kUnstoppableDomains =
    base::MakeFixedFlatSet<std::string_view>(
        {".altimist",   ".anime",       ".ask",       ".austin",
         ".bald",       ".basenji",     ".bay",       ".benji",
         ".binanceus",  ".bitcoin",     ".bitget",    ".bitscrunch",
         ".blockchain", ".boomer",      ".brave",     ".calicoin",
         ".caw",        ".chomp",       ".clay",      ".crypto",
         ".dao",        ".dfz",         ".doga",      ".donut",
         ".dream",      ".emir",        ".ethermail", ".farms",
         ".grow",       ".her",         ".kingdom",   ".klever",
         ".kresus",     ".kryptic",     ".lfg",       ".ltc",
         ".manga",      ".metropolis",  ".miku",      ".ministry",
         ".moon",       ".mumu",        ".nft",       ".nibi",
         ".npc",        ".onchain",     ".pastor",    ".podcast",
         ".pog",        ".polygon",     ".privacy",   ".propykeys",
         ".pudgy",      ".quantum",     ".rad",       ".raiin",
         ".secret",     ".smobler",     ".south",     ".stepn",
         ".tball",      ".tea",         ".tribe",     ".u",
         ".ubu",        ".unstoppable", ".wallet",    ".wifi",
         ".witg",       ".wrkx",        ".x",         ".xec",
         ".xmr",        ".zil"},
        SuffixComparator());

// Ensure all domain suffixes start with `.`
constexpr bool CheckAllDomainSuffixesStartWithDot() {
  for (const auto& domain : kUnstoppableDomains) {
    if (domain[0] != '.' && std::ranges::count(domain, '.') == 1) {
      return false;
    }
  }
  return true;
}
static_assert(CheckAllDomainSuffixesStartWithDot(),
              "kUnstoppableDomains must start with a '.', and only one "
              "occurrence of the '.'");

}  // namespace

std::optional<std::string_view> GetUnstoppableDomainSuffix(
    std::string_view host) {
  auto domain = kUnstoppableDomains.find(DomainSuffixLookup(host));
  if (domain == kUnstoppableDomains.end()) {
    return std::nullopt;
  }
  return *domain;
}

std::string GetUnstoppableDomainSuffixFullList() {
  return base::JoinString(kUnstoppableDomains, ", ");
}

}  // namespace decentralized_dns
