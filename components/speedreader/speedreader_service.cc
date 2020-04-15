/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_service.h"

#include "base/command_line.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace speedreader {

SpeedreaderService::SpeedreaderService(PrefService* prefs) : prefs_(prefs) {}

SpeedreaderService::~SpeedreaderService() {}

// static
void SpeedreaderService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kSpeedreaderEnabled, false);
}

void SpeedreaderService::ToggleSpeedreader() {
  const bool enabled = prefs_->GetBoolean(kSpeedreaderEnabled);
  prefs_->SetBoolean(kSpeedreaderEnabled, !enabled);
}

bool SpeedreaderService::IsEnabled() {
  const auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (!cmd_line->HasSwitch(kEnableSpeedreader)) {
    return false;
  }

  return prefs_->GetBoolean(kSpeedreaderEnabled);
}

}  // namespace speedreader
