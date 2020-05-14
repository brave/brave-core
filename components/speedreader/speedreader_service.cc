/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_service.h"

#include "base/feature_list.h"
#include "brave/components/speedreader/features.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace speedreader {

SpeedreaderService::SpeedreaderService(PrefService* prefs) : prefs_(prefs) {}

SpeedreaderService::~SpeedreaderService() {}

// static
void SpeedreaderService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kSpeedreaderPrefEnabled, false);
}

void SpeedreaderService::ToggleSpeedreader() {
  const bool enabled = prefs_->GetBoolean(kSpeedreaderPrefEnabled);
  prefs_->SetBoolean(kSpeedreaderPrefEnabled, !enabled);
}

bool SpeedreaderService::IsEnabled() {
  if (!base::FeatureList::IsEnabled(kSpeedreaderFeature)) {
    return false;
  }

  return prefs_->GetBoolean(kSpeedreaderPrefEnabled);
}

}  // namespace speedreader
