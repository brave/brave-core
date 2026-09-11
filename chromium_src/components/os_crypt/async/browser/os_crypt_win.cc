// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>
#include <string_view>

#include "components/prefs/pref_service.h"

namespace os_crypt_async {
namespace {

// `Local State` holds the only copy of the key that every saved password,
// cookie and payment method is encrypted with, and nothing else on disk can
// reconstruct it. Replacing that key orphans all of it, silently, so the guards
// below make sure it is only ever replaced when the profile genuinely does not
// have one. See https://github.com/brave/brave-browser/issues/40375.

// Shadows `os_crypt.encrypted_key`.
inline constexpr char kBraveEncryptedKeyBackupPrefName[] =
    "os_crypt.encrypted_key_backup";

// Whether a missing key can be taken to mean "this profile does not have one
// yet".
//
// `HasPrefPath()` cannot tell that apart from "`Local State` could not be
// read". A file that is corrupt (`JsonPrefStore` moves it aside as `Local
// State.bad` and carries on with empty preferences), locked by another process,
// or unreadable all arrive as an empty pref set, and reading that as a first
// run mints a new key over one that is still sitting on disk.
bool BraveCanConcludeKeyIsAbsent(PrefService* local_state) {
  switch (local_state->GetInitializationStatus()) {
    case PrefService::INITIALIZATION_STATUS_SUCCESS:
      return true;
    case PrefService::INITIALIZATION_STATUS_CREATED_NEW_PREF_STORE:
      // `Local State` was absent rather than unreadable, so this is a first run
      // and there is no key to lose.
      return true;
    case PrefService::INITIALIZATION_STATUS_WAITING:
    case PrefService::INITIALIZATION_STATUS_ERROR:
      return false;
  }
}

// Copies the current key aside before it is replaced, so a key we turn out to
// have replaced in error can still be recovered by hand. An existing backup is
// never overwritten: the oldest key we ever saw is the one worth keeping, since
// it is the one the bulk of the data was encrypted with.
void BraveBackUpKeyBeforeReplacing(PrefService* local_state,
                                   std::string_view pref_name) {
  const std::string current = local_state->GetString(pref_name);
  if (current.empty() ||
      !local_state->GetString(kBraveEncryptedKeyBackupPrefName).empty()) {
    return;
  }
  local_state->SetString(kBraveEncryptedKeyBackupPrefName, current);
}

}  // namespace
}  // namespace os_crypt_async

#include <components/os_crypt/async/browser/os_crypt_win.cc>
