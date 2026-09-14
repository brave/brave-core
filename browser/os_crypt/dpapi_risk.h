/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_OS_CRYPT_DPAPI_RISK_H_
#define BRAVE_BROWSER_OS_CRYPT_DPAPI_RISK_H_

#include <stdint.h>

class PrefRegistrySimple;
class PrefService;

namespace brave {

// Conditions that can leave Windows unable to unwrap the DPAPI master key.
//
// On Windows every saved password, cookie and payment method is encrypted with
// a key held in `os_crypt.encrypted_key` in `Local State`, and that key is
// wrapped by DPAPI. `CryptProtectData()` and `CryptUnprotectData()` take no key
// argument: Windows derives a per-user master key from the user's credentials
// and keeps it under `%APPDATA%\Microsoft\Protect\<SID>\`. If that master key
// becomes unusable, every blob wrapped with it becomes unreadable at once, and
// no copy Brave could keep would help, since any such copy is itself wrapped by
// the same master key.
//
// None of the signals below cause the loss. They describe configurations in
// which the known causes are more likely, so that a later change can offer the
// user guidance before it happens rather than after.
//
// The known causes, recorded here because they are also what support needs:
//
//  * An administrator resets the user's password. A user changing their own
//    password lets Windows re-wrap the master keys with a key derived from the
//    new password. An administrator reset cannot, because Windows never had the
//    old password. Domain accounts may recover through the domain backup key;
//    local accounts generally cannot. This is the canonical cause.
//  * The profile is opened under a different Windows account, or on another
//    machine. Master keys are per-SID and stored locally, so a copied or
//    restored `User Data` directory cannot be unwrapped.
//  * `%APPDATA%\Microsoft\Protect` is deleted, which some cleanup and privacy
//    tools do.
//
// Routine master key rotation, roughly every 90 days by default, is *not* a
// cause: Windows retains the older master keys so existing blobs keep working.
//
// TODO(https://github.com/brave/brave-browser/issues/40375): the following are
// believed to be causes but are not yet confirmed, and should be verified
// before they reach a support article. A local account being linked to a
// Microsoft account is believed to keep the same SID, and so keep DPAPI intact,
// while a newly created Microsoft-account profile does not; "Reset this PC"
// keeping files is expected to produce a new SID; and a domain machine unable
// to reach its backup key is believed to matter mainly in combination with an
// administrator password reset.
enum class DPAPIRiskSignal : uint32_t {
  // An administrator can reset this user's password, which is the canonical way
  // to lose the master key.
  kDomainJoined = 1 << 0,
  kAzureADJoined = 1 << 1,
  kDeviceManaged = 1 << 2,
  // The profile lives somewhere more than one machine can reach, so it may be
  // opened under a different SID than the one that wrapped the key.
  kUserDataDirOnNetwork = 1 << 3,
  // Weaker than the ones above, and worth reading with care. Chromium disables
  // app-bound encryption for roaming profiles
  // (`GetAppBoundEncryptionSupportLevel()` in
  // chrome/browser/os_crypt/app_bound_encryption_win.cc) because app-bound
  // binds to the *SYSTEM* DPAPI key, which does not roam. The key this file is
  // concerned with is wrapped by the *user* master key, which lives under
  // %APPDATA% and does roam with the profile. These are recorded because
  // partial or conflicting roaming can still damage the Protect folder, not
  // because roaming by itself moves the key out of reach.
  kRoamingWindowsProfile = 1 << 4,
  kFSLogix = 1 << 5,
};

// What the machine looks like. Split out from the signal mapping so the mapping
// can be tested without a domain-joined bot.
struct DPAPIEnvironment {
  bool domain_joined = false;
  bool azure_ad_joined = false;
  bool device_managed = false;
  bool user_data_dir_on_network = false;
  bool roaming_windows_profile = false;
  bool fslogix_present = false;
};

// Maps an environment onto a bitmask of `DPAPIRiskSignal`.
uint32_t ComputeDPAPIRiskSignals(const DPAPIEnvironment& environment);

// Queries Windows. Blocking: `IsEnrolledToDomain()` wraps
// `NetGetJoinInformation()`, which can be slow on a domain-joined machine, so
// never call this on the critical path.
DPAPIEnvironment DetectDPAPIEnvironment();

void RegisterDPAPIRiskLocalStatePrefs(PrefRegistrySimple* registry);

// Detects the environment on a blocking background task and records the result
// in local state. Nothing is reported anywhere and nothing is shown to the
// user; a later change reads these prefs and decides what to do about them.
void RecordDPAPIRiskSignals(PrefService* local_state);

}  // namespace brave

#endif  // BRAVE_BROWSER_OS_CRYPT_DPAPI_RISK_H_
