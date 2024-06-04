/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/decentralized_dns/core/utils.h"

#include <string_view>

#include "base/strings/string_util.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/net/decentralized_dns/constants.h"
#include "components/base32/base32.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace {
// Ipfs codes from multicodec table
// https://github.com/multiformats/multicodec/blob/master/table.csv
const int64_t kIpfsNSCodec = 0xE3;
const int64_t kIpnsNSCodec = 0xE5;
inline constexpr char kIPFSScheme[] = "ipfs";
inline constexpr char kIPNSScheme[] = "ipns";

// Decodes a varint from the given string piece into the given int64_t. Returns
// remaining span if the string had a valid varint (where a byte was found with
// it's top bit set).
base::span<const uint8_t> DecodeVarInt(base::span<const uint8_t> from,
                                       int64_t* into) {
  auto it = from.begin();
  int shift = 0;
  uint64_t ret = 0;
  do {
    if (it == from.end()) {
      return {};
    }

    // Shifting 64 or more bits is undefined behavior.
    DCHECK_LT(shift, 64);
    unsigned char c = *it;
    ret |= static_cast<uint64_t>(c & 0x7f) << shift;
    shift += 7;
  } while (*it++ & 0x80);
  *into = static_cast<int64_t>(ret);
  return from.subspan(it - from.begin());
}

}  // namespace
namespace decentralized_dns {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kUnstoppableDomainsResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
  registry->RegisterIntegerPref(kENSResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));
  registry->RegisterIntegerPref(
      kEnsOffchainResolveMethod,
      static_cast<int>(EnsOffchainResolveMethod::kAsk));
  registry->RegisterIntegerPref(kSnsResolveMethod,
                                static_cast<int>(ResolveMethodTypes::ASK));

  // Register prefs for migration.
  // Added 12/2023 to reset SNS pref to re-opt in with updated interstitial.
  registry->RegisterBooleanPref(kSnsResolveMethodMigrated, false);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 05/2022
  if (static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS) ==
      local_state->GetInteger(kUnstoppableDomainsResolveMethod)) {
    local_state->ClearPref(kUnstoppableDomainsResolveMethod);
  }
  if (static_cast<int>(ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS) ==
      local_state->GetInteger(kENSResolveMethod)) {
    local_state->ClearPref(kENSResolveMethod);
  }

  // Added 12/2023
  // Reset SNS resolve method to ask to re-opt in with updated interstitial.
  if (!local_state->GetBoolean(kSnsResolveMethodMigrated)) {
    if (local_state->GetInteger(kSnsResolveMethod) ==
        static_cast<int>(ResolveMethodTypes::ENABLED)) {
      local_state->ClearPref(kSnsResolveMethod);
    }
    local_state->SetBoolean(kSnsResolveMethodMigrated, true);
  }
}

bool IsUnstoppableDomainsTLD(const std::string_view host) {
  for (auto* domain : kUnstoppableDomains) {
    if (base::EndsWith(host, domain)) {
      return true;
    }
  }
  return false;
}

void SetUnstoppableDomainsResolveMethod(PrefService* local_state,
                                        ResolveMethodTypes method) {
  local_state->SetInteger(kUnstoppableDomainsResolveMethod,
                          static_cast<int>(method));
}

ResolveMethodTypes GetUnstoppableDomainsResolveMethod(
    PrefService* local_state) {
  return static_cast<ResolveMethodTypes>(
      local_state->GetInteger(kUnstoppableDomainsResolveMethod));
}

bool IsUnstoppableDomainsResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetUnstoppableDomainsResolveMethod(local_state) ==
         ResolveMethodTypes::ASK;
}

bool IsUnstoppableDomainsResolveMethodEnabled(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetUnstoppableDomainsResolveMethod(local_state) ==
         ResolveMethodTypes::ENABLED;
}

bool IsENSTLD(const std::string_view host) {
  return base::EndsWith(host, kEthDomain);
}

void SetENSResolveMethod(PrefService* local_state, ResolveMethodTypes method) {
  local_state->SetInteger(kENSResolveMethod, static_cast<int>(method));
}

ResolveMethodTypes GetENSResolveMethod(PrefService* local_state) {
  return static_cast<ResolveMethodTypes>(
      local_state->GetInteger(kENSResolveMethod));
}

bool IsENSResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetENSResolveMethod(local_state) == ResolveMethodTypes::ASK;
}

bool IsENSResolveMethodEnabled(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetENSResolveMethod(local_state) == ResolveMethodTypes::ENABLED;
}

void SetEnsOffchainResolveMethod(PrefService* local_state,
                                 EnsOffchainResolveMethod method) {
  local_state->SetInteger(kEnsOffchainResolveMethod, static_cast<int>(method));
}

EnsOffchainResolveMethod GetEnsOffchainResolveMethod(PrefService* local_state) {
  return static_cast<EnsOffchainResolveMethod>(
      local_state->GetInteger(kEnsOffchainResolveMethod));
}

bool IsSnsTLD(const std::string_view host) {
  return base::EndsWith(host, kSolDomain);
}

void SetSnsResolveMethod(PrefService* local_state, ResolveMethodTypes method) {
  local_state->SetInteger(kSnsResolveMethod, static_cast<int>(method));
}

ResolveMethodTypes GetSnsResolveMethod(PrefService* local_state) {
  return static_cast<ResolveMethodTypes>(
      local_state->GetInteger(kSnsResolveMethod));
}

bool IsSnsResolveMethodAsk(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetSnsResolveMethod(local_state) == ResolveMethodTypes::ASK;
}

bool IsSnsResolveMethodEnabled(PrefService* local_state) {
  if (!local_state) {
    return false;  // Treat it as disabled.
  }

  return GetSnsResolveMethod(local_state) == ResolveMethodTypes::ENABLED;
}

GURL ContentHashToCIDv1URL(base::span<const uint8_t> contenthash) {
  int64_t code = 0;
  contenthash = DecodeVarInt(contenthash, &code);
  if (contenthash.empty()) {
    return GURL();
  }
  if (code != kIpnsNSCodec && code != kIpfsNSCodec) {
    return GURL();
  }
  std::string encoded = base32::Base32Encode(contenthash);
  if (encoded.empty()) {
    return GURL();
  }
  std::string trimmed;
  base::TrimString(encoded, "=", &trimmed);
  std::string lowercase = base::ToLowerASCII(trimmed);
  // multibase format <base-encoding-character><base-encoded-data>
  // https://github.com/multiformats/multibase/blob/master/multibase.csv
  std::string cidv1 = "b" + lowercase;
  std::string scheme = (code == kIpnsNSCodec) ? kIPNSScheme : kIPFSScheme;
  return GURL(scheme + "://" + cidv1);
}

}  // namespace decentralized_dns
