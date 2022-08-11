/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_farbling_service.h"

#include <string>

#include "base/feature_list.h"
#include "base/rand_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "crypto/hmac.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace brave {

BraveFarblingService::BraveFarblingService() {
  // initialize random seeds for farbling
  session_token_ = base::RandUint64();
  incognito_session_token_ = base::RandUint64();
}

BraveFarblingService::~BraveFarblingService() = default;

uint64_t BraveFarblingService::session_token(bool is_off_the_record) {
  if (is_off_the_record)
    return incognito_session_token_;
  return session_token_;
}

void BraveFarblingService::set_session_tokens_for_testing(
    uint64_t session_token,
    uint64_t incognito_session_token) {
  session_token_ = session_token;
  incognito_session_token_ = incognito_session_token;
}

bool BraveFarblingService::MakePseudoRandomGeneratorForURL(
    const GURL& url,
    bool is_off_the_record,
    FarblingPRNG* prng) {
  const std::string domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (domain.empty())
    return false;
  uint8_t domain_key[32];
  uint64_t session_key = session_token(is_off_the_record);
  crypto::HMAC h(crypto::HMAC::SHA256);
  CHECK(h.Init(reinterpret_cast<const unsigned char*>(&session_key),
               sizeof session_key));
  CHECK(h.Sign(domain, domain_key, sizeof domain_key));
  uint64_t seed = *reinterpret_cast<uint64_t*>(domain_key);
  *prng = FarblingPRNG(seed);
  return true;
}

// static
void BraveFarblingService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(brave_shields::prefs::kReduceLanguageEnabled,
                                true);
}

}  // namespace brave
