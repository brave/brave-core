// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include "base/check.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "url/gurl.h"

namespace brave {

BraveFarblingService::BraveFarblingService(
    brave_shields::BraveShieldsSettingsService* brave_shields_settings_service)
    : brave_shields_settings_service_(brave_shields_settings_service) {
  DCHECK(brave_shields_settings_service_);
}

BraveFarblingService::~BraveFarblingService() = default;

bool BraveFarblingService::MakePseudoRandomGeneratorForURL(
    const GURL& url,
    base::span<const uint8_t> additional_entropy,
    FarblingPRNG* prng) {
  if (brave_shields_settings_service_->GetFarblingLevel(url) ==
      brave_shields::mojom::FarblingLevel::OFF) {
    return false;
  }
  const base::Token farbling_token =
      brave_shields_settings_service_->GetFarblingToken(url,
                                                        additional_entropy);
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

}  // namespace brave
