// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include "base/check.h"
#include "base/hash/hash.h"
#include "base/token.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "net/base/schemeful_site.h"
#include "url/gurl.h"

namespace brave_shields {
// Used for stable farbling token generation in tests when is set to non-zero.
// Non-anonymous to be accesible from ":test_support" target.
uint32_t g_stable_farbling_tokens_seed = 0;
}  // namespace brave_shields

namespace brave {

namespace {

base::Token CreateStableFarblingToken(const GURL& site_url) {
  const uint32_t high = base::PersistentHash(site_url.host()) +
                        brave_shields::g_stable_farbling_tokens_seed - 1;
  const uint32_t low = base::PersistentHash(base::byte_span_from_ref(high));
  return base::Token(high, low);
}

// Returns a 64-bit persistent hash of |data| (two rounds of PersistentHash).
uint64_t PersistentHashU64(base::span<const uint8_t> data) {
  const uint32_t hash = base::PersistentHash(data);
  return (static_cast<uint64_t>(hash) << 32) |
         base::PersistentHash(base::byte_span_from_ref(hash));
}

}  // namespace

BraveFarblingService::BraveFarblingService(
    HostContentSettingsMap* host_content_settings_map)
    : host_content_settings_map_(host_content_settings_map) {
  DCHECK(host_content_settings_map_);
}

BraveFarblingService::~BraveFarblingService() = default;

bool BraveFarblingService::MakePseudoRandomGeneratorForURL(
    const GURL& url,
    base::span<const uint8_t> additional_entropy,
    FarblingPRNG* prng) {
  if (brave_shields::GetFarblingLevel(host_content_settings_map_, url) ==
      brave_shields::mojom::FarblingLevel::OFF) {
    return false;
  }
  const base::Token farbling_token = GetFarblingToken(url, additional_entropy);
  if (farbling_token.is_zero()) {
    return false;
  }
  *prng = FarblingPRNG(farbling_token.high() ^ farbling_token.low());
  return true;
}

// static
void BraveFarblingService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(brave_shields::prefs::kReduceLanguageEnabled,
                                true);
}

// TODO(https://github.com/brave/brave-browser/issues/56805): Update this method
// to take in url::Origin
base::Token BraveFarblingService::GetFarblingToken(
    const GURL& url,
    base::span<const uint8_t> additional_entropy) {
  base::Token token;
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return token;
  }

  // Normalize to the schemeful site (scheme + eTLD+1) so that all subdomains
  // and paths of the same site share one token.
  const GURL site_url = net::SchemefulSite(url).GetURL();
  const std::string key = site_url.spec();
  if (key.empty()) {
    return token;
  }

  auto it = farbling_tokens_map_.find(key);
  if (it != farbling_tokens_map_.end()) {
    token = it->second;
  } else {
    if (!brave_shields::g_stable_farbling_tokens_seed) {
      token = base::Token::CreateRandom();
    } else {
      token = CreateStableFarblingToken(site_url);
    }
    farbling_tokens_map_.emplace(key, token);
  }

  if (additional_entropy.empty()) {
    return token;
  }

  const uint64_t high = token.high() ^ PersistentHashU64(additional_entropy);
  const uint64_t low =
      token.low() ^ PersistentHashU64(base::byte_span_from_ref(high));
  return base::Token(high, low);
}

void BraveFarblingService::ResetFarblingTokensMap() {
  farbling_tokens_map_.clear();
}

}  // namespace brave
