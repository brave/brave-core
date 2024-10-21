// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "url/gurl.h"

namespace brave {

BraveFarblingService::BraveFarblingService(
    HostContentSettingsMap* host_content_settings_map)
    : host_content_settings_map_(host_content_settings_map) {
  DCHECK(host_content_settings_map_);
}

BraveFarblingService::~BraveFarblingService() = default;

bool BraveFarblingService::MakePseudoRandomGeneratorForURL(const GURL& url,
                                                           FarblingPRNG* prng) {
  if (brave_shields::GetFarblingLevel(host_content_settings_map_, url) ==
      brave_shields::mojom::FarblingLevel::OFF) {
    return false;
  }
  const base::Token farbling_token =
      brave_shields::GetFarblingToken(host_content_settings_map_, url);
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
