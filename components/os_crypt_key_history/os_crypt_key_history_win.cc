/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/os_crypt_key_history/os_crypt_key_history_win.h"

#include <windows.h>

#include <wincrypt.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/os_crypt_key_history/os_crypt_key_history_recorder.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

// Both defined privately in
// components/os_crypt/async/browser/os_crypt_win.cc, and repeated here rather
// than shared because that file exposes neither.
constexpr char kOsCryptEncryptedKeyPrefName[] = "os_crypt.encrypted_key";
constexpr char kDPAPIKeyPrefix[] = "DPAPI";

// The name this provider's keys are filed under in the history. Only DPAPI is
// recorded for now: the app-bound key can only be unwrapped by the elevation
// service, so there is no way to tell two of them apart here.
constexpr char kDPAPIProvider[] = "dpapi";

// Unwraps a key as `Local State` stores it. Mirrors DecryptStringWithDPAPI()
// in components/os_crypt/async/browser/os_crypt_win.cc.
std::optional<std::string> UnwrapKey(std::string_view base64_wrapped_key) {
  std::string wrapped_key;
  if (!base::Base64Decode(base64_wrapped_key, &wrapped_key) ||
      !base::StartsWith(wrapped_key, kDPAPIKeyPrefix,
                        base::CompareCase::SENSITIVE)) {
    return std::nullopt;
  }
  const std::string ciphertext =
      wrapped_key.substr(sizeof(kDPAPIKeyPrefix) - 1);

  DATA_BLOB input = {};
  input.pbData =
      const_cast<BYTE*>(reinterpret_cast<const BYTE*>(ciphertext.data()));
  input.cbData = static_cast<DWORD>(ciphertext.size());

  DATA_BLOB output = {};
  if (!::CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0,
                            &output)) {
    return std::nullopt;
  }

  std::string plaintext(reinterpret_cast<char*>(output.pbData), output.cbData);
  ::SecureZeroMemory(output.pbData, output.cbData);
  ::LocalFree(output.pbData);
  return plaintext;
}

// Two wrapped keys hold the same key material when they unwrap to the same
// bytes. Their wrapped bytes will differ even so: DPAPI adds fresh entropy
// every time, and OSCrypt re-wraps the key when it turns auditing on.
bool SameWrappedKey(std::string_view current, std::string_view stored) {
  const std::optional<std::string> current_key = UnwrapKey(current);
  if (!current_key) {
    return false;
  }
  const std::optional<std::string> stored_key = UnwrapKey(stored);
  return stored_key && *stored_key == *current_key;
}

}  // namespace

void InitializeOSCryptKeyHistoryRecorder(const base::FilePath& user_data_dir,
                                         PrefService* local_state) {
  if (user_data_dir.empty() || !local_state) {
    return;
  }

  const std::string wrapped_key =
      local_state->GetString(kOsCryptEncryptedKeyPrefName);
  if (wrapped_key.empty()) {
    // Nothing worth recording yet. A key minted later this session has not
    // decrypted anything, which is exactly what must not be recorded.
    return;
  }

  os_crypt_key_history_recorder::Initialize(
      user_data_dir.Append(kOSCryptKeyHistoryFileName),
      {{kDPAPIProvider, wrapped_key}}, base::BindRepeating(&SameWrappedKey));
}

}  // namespace brave
