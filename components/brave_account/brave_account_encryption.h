/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_ENCRYPTION_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_ENCRYPTION_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace brave_account {

// Encrypts/decrypts Brave Account data via `os_crypt_async::Encryptor`, with
// base64-encoded ciphertext so values can safely round-trip through prefs/JSON.
// The encryptor is held by `raw_ref` and must outlive this instance.
// `SetOSCryptCallbacksForTesting()` allows tests to stub the underlying
// OSCrypt primitives directly, e.g. to inject failures.
class BraveAccountEncryption {
 public:
  using OSCryptCallback =
      base::RepeatingCallback<bool(const std::string&, std::string*)>;

  static void SetOSCryptCallbacksForTesting(OSCryptCallback encrypt_callback,
                                            OSCryptCallback decrypt_callback);

  explicit BraveAccountEncryption(const os_crypt_async::Encryptor& encryptor);

  BraveAccountEncryption(const BraveAccountEncryption&) = delete;
  BraveAccountEncryption& operator=(const BraveAccountEncryption&) = delete;

  std::string Encrypt(const std::string& plain_text) const;
  std::string Decrypt(const std::string& base64) const;

 private:
  static OSCryptCallback& EncryptCallbackForTesting();
  static OSCryptCallback& DecryptCallbackForTesting();

  const raw_ref<const os_crypt_async::Encryptor> encryptor_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_ENCRYPTION_H_
