/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service.h"

#include <string>

#include "brave/browser/brave_browser_process_impl.h"
// TODO(bridiver) - move this out of extensions
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "brave/common/tor/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace tor {

TorProfileService::TorProfileService() {
}

TorProfileService::~TorProfileService() {
}

// static
void TorProfileService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kTorDisabled, false);
}

// static
bool TorProfileService::IsTorDisabled() {
  if (!g_brave_browser_process)
    return false;
  return g_browser_process->local_state()->GetBoolean(prefs::kTorDisabled);
}

// static
void TorProfileService::SetTorDisabled(bool disabled) {
  if (g_brave_browser_process)
    g_browser_process->local_state()->SetBoolean(prefs::kTorDisabled, disabled);
}

// static
void TorProfileService::RegisterTorClientUpdater() {
  if (g_brave_browser_process) {
    g_brave_browser_process->tor_client_updater()->Register();
  }
}

// static
void TorProfileService::UnregisterTorClientUpdater() {
  if (g_brave_browser_process) {
    g_brave_browser_process->tor_client_updater()->Unregister();
  }
}

base::FilePath TorProfileService::GetTorExecutablePath() {
  return g_brave_browser_process->tor_client_updater()->GetExecutablePath();
}

void TorProfileService::AddObserver(TorLauncherServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void TorProfileService::RemoveObserver(TorLauncherServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace tor
