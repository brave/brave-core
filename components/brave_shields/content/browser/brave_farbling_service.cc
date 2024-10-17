// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include <string>

#include "base/feature_list.h"
#include "base/numerics/byte_conversions.h"
#include "base/rand_util.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "crypto/hmac.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace brave {

BraveFarblingService::BraveFarblingService() {
  // initialize random seeds for farbling
  session_token_ = base::RandUint64();
}

BraveFarblingService::~BraveFarblingService() = default;

uint64_t BraveFarblingService::session_token() {
  return session_token_;
}

void BraveFarblingService::set_session_tokens_for_testing(
    uint64_t session_token) {
  session_token_ = session_token;
}

bool BraveFarblingService::MakePseudoRandomGeneratorForURL(const GURL& url,
                                                           FarblingPRNG* prng) {
  const std::string domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (domain.empty()) {
    return false;
  }
  uint8_t domain_key[32];
  uint64_t session_key = session_token();
  crypto::HMAC h(crypto::HMAC::SHA256);
  CHECK(h.Init(base::byte_span_from_ref(session_key)));
  CHECK(h.Sign(base::as_byte_span(domain),
               base::as_writable_byte_span(domain_key)));
  *prng = FarblingPRNG(
      base::U64FromNativeEndian(base::as_byte_span(domain_key).first<8>()));
  return true;
}

// static
void BraveFarblingService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(brave_shields::prefs::kReduceLanguageEnabled,
                                true);
}

}  // namespace brave
