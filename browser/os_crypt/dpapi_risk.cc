/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/dpapi_risk.h"

#include <windows.h>

#include <userenv.h>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr char kRiskSignalsPrefName[] = "brave.os_crypt.dpapi_risk_signals";
constexpr char kLastComputedPrefName[] =
    "brave.os_crypt.dpapi_risk_last_computed";

// A roaming profile solution that does not set a Windows profile type, so it
// has to be looked for directly. Chromium checks the same key in
// `GetAppBoundEncryptionSupportLevel()`.
constexpr wchar_t kFSLogixKeyPath[] = L"SOFTWARE\\FSLogix";

bool IsFSLogixPresent() {
  for (const auto access_mask : {KEY_WOW64_32KEY, KEY_WOW64_64KEY}) {
    if (base::win::RegKey{}.Open(HKEY_LOCAL_MACHINE, kFSLogixKeyPath,
                                 KEY_QUERY_VALUE | access_mask) ==
        ERROR_SUCCESS) {
      return true;
    }
  }
  return false;
}

void OnEnvironmentDetected(PrefService* local_state,
                           const DPAPIEnvironment& environment) {
  local_state->SetInteger(
      kRiskSignalsPrefName,
      static_cast<int>(ComputeDPAPIRiskSignals(environment)));
  local_state->SetTime(kLastComputedPrefName, base::Time::Now());
}

}  // namespace

uint32_t ComputeDPAPIRiskSignals(const DPAPIEnvironment& environment) {
  uint32_t signals = 0;
  auto add = [&signals](bool present, DPAPIRiskSignal signal) {
    if (present) {
      signals |= static_cast<uint32_t>(signal);
    }
  };

  add(environment.domain_joined, DPAPIRiskSignal::kDomainJoined);
  add(environment.azure_ad_joined, DPAPIRiskSignal::kAzureADJoined);
  add(environment.device_managed, DPAPIRiskSignal::kDeviceManaged);
  add(environment.user_data_dir_on_network,
      DPAPIRiskSignal::kUserDataDirOnNetwork);
  add(environment.roaming_windows_profile,
      DPAPIRiskSignal::kRoamingWindowsProfile);
  add(environment.fslogix_present, DPAPIRiskSignal::kFSLogix);

  return signals;
}

DPAPIEnvironment DetectDPAPIEnvironment() {
  DPAPIEnvironment environment;

  // Each of these means an administrator holds credentials that can be reset
  // out from under the user, which is the canonical way the master key is lost.
  environment.domain_joined = base::win::IsEnrolledToDomain();
  environment.azure_ad_joined = base::win::IsJoinedToAzureAD();
  environment.device_managed = base::win::IsDeviceRegisteredWithManagement();

  environment.user_data_dir_on_network =
      base::PathService::CheckedGet(chrome::DIR_USER_DATA).IsNetwork();

  // Anything other than a local profile: roaming, mandatory or temporary.
  DWORD profile_type = 0;
  if (::GetProfileType(&profile_type)) {
    environment.roaming_windows_profile = profile_type > 0;
  }

  environment.fslogix_present = IsFSLogixPresent();

  return environment;
}

void RegisterDPAPIRiskLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kRiskSignalsPrefName, 0);
  registry->RegisterTimePref(kLastComputedPrefName, base::Time());
}

void RecordDPAPIRiskSignals(PrefService* local_state) {
  if (!local_state) {
    return;
  }

  // Recomputed every launch rather than cached: domain join and roaming state
  // do change, and the answer is only useful if it is current.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&DetectDPAPIEnvironment),
      // `local_state` is owned by BrowserProcessImpl and outlives every
      // task posted here; SKIP_ON_SHUTDOWN keeps this from running once
      // teardown has begun.
      base::BindOnce(&OnEnvironmentDetected, base::Unretained(local_state)));
}

}  // namespace brave
