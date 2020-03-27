/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_service.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "brave/components/speedreader/speedreader_pref_names.h"

namespace speedreader {

SpeedreaderService::SpeedreaderService(PrefService* prefs) : prefs_(prefs) {
}

SpeedreaderService::~SpeedreaderService() {}

// static
void SpeedreaderService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(speedreader::kSpeedreaderEnabled, false);
}

void SpeedreaderService::ToggleSpeedreader() {
  const bool enabled = prefs_->GetBoolean(speedreader::kSpeedreaderEnabled);
  prefs_->SetBoolean(speedreader::kSpeedreaderEnabled, !enabled);
}

bool SpeedreaderService::IsEnabled() {
  return prefs_->GetBoolean(speedreader::kSpeedreaderEnabled);
}


}  // namespace speedreader
