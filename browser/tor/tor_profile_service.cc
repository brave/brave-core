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
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace tor {

namespace {

constexpr char kTorProxyScheme[] = "socks5://";
constexpr char kTorProxyAddress[] = "127.0.0.1";

}  // namespace

TorProfileService::TorProfileService() {
}

TorProfileService::~TorProfileService() {
}

// static
void TorProfileService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  std::string port;
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      port = std::string("9350");
      break;
    case version_info::Channel::BETA:
      port = std::string("9360");
      break;
    case version_info::Channel::DEV:
      port = std::string("9370");
      break;
    case version_info::Channel::CANARY:
      port = std::string("9380");
      break;
    case version_info::Channel::UNKNOWN:
    default:
      port = std::string("9390");
  }
  const std::string tor_proxy_uri =
      std::string(kTorProxyScheme) + std::string(kTorProxyAddress) + ":" + port;
  registry->RegisterStringPref(prefs::kTorProxyString, tor_proxy_uri);
  // kTorDisabled is managed prefs.
  // User can only change it via settiongs option if it's not managed.
  registry->RegisterBooleanPref(prefs::kTorDisabled, false);
  // kTorDisabledAtNextLaunching has the value that user changed via settings.
  // If this value is changed, it is exposed to settings page. At the next
  // launching, this value is copied to kTorDisabled.
  registry->RegisterBooleanPref(prefs::kTorDisabledAtNextLaunching, false);
}

// static
void TorProfileService::InitializeTorPrefs() {
  auto* state = g_browser_process->local_state();
  if (state->FindPreference(
          prefs::kTorDisabledAtNextLaunching)->IsDefaultValue()) {
    return;
  }

  state->SetBoolean(prefs::kTorDisabled,
                    state->GetBoolean(prefs::kTorDisabledAtNextLaunching));
  state->ClearPref(prefs::kTorDisabledAtNextLaunching);
}

// static
bool TorProfileService::IsTorDisabled() {
  return g_browser_process->local_state()->GetBoolean(prefs::kTorDisabled);
}

// static
bool TorProfileService::IsTorDisabledAtNextLaunching() {
  return g_browser_process->local_state()->GetBoolean(
      prefs::kTorDisabledAtNextLaunching);
}

// static
bool TorProfileService::IsTorDisabledChanged() {
  auto* state = g_browser_process->local_state();
  if (state->FindPreference(
         prefs::kTorDisabledAtNextLaunching)->IsDefaultValue()) {
    return false;
  }

  return state->GetBoolean(prefs::kTorDisabledAtNextLaunching) !=
         state->GetBoolean(prefs::kTorDisabled);
}

// static
void TorProfileService::SetTorDisabledAtNextLaunching(bool disabled) {
  g_browser_process->local_state()->SetBoolean(
      prefs::kTorDisabledAtNextLaunching, disabled);
}

std::string TorProfileService::GetTorProxyURI() {
  return g_browser_process->local_state()->GetString(prefs::kTorProxyString);
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
