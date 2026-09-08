// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <string_view>

#include "components/prefs/pref_service.h"

namespace os_crypt_async {
namespace {

// Shadows `os_crypt.app_bound_encrypted_key`. The companion guards for the
// DPAPI key live in
// //brave/chromium_src/components/os_crypt/async/browser/os_crypt_win.cc.
// See https://github.com/brave/brave-browser/issues/40375.
inline constexpr char kBraveAppBoundEncryptedKeyBackupPrefName[] =
    "os_crypt.app_bound_encrypted_key_backup";

// Copies the current app-bound key aside before it is replaced. This key still
// has a live replacement path that the DPAPI one no longer has:
// `StoreAndReplyWithKey()` mints a fresh key whenever a decryption failure is
// classified as permanent, and everything encrypted under the old key is
// orphaned when it does.
void BraveBackUpAppBoundKeyBeforeReplacing(PrefService* local_state,
                                           std::string_view pref_name) {
  const std::string current = local_state->GetString(pref_name);
  if (current.empty() ||
      !local_state->GetString(kBraveAppBoundEncryptedKeyBackupPrefName)
           .empty()) {
    return;
  }
  local_state->SetString(kBraveAppBoundEncryptedKeyBackupPrefName, current);
}

}  // namespace
}  // namespace os_crypt_async

#include <chrome/browser/os_crypt/app_bound_encryption_provider_win.cc>
