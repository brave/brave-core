/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_encryption.h"

#include <utility>

#include "base/base64.h"
#include "base/check_is_test.h"
#include "base/no_destructor.h"

namespace brave_account {

// static
void BraveAccountEncryption::SetOSCryptCallbacksForTesting(  // IN-TEST
    OSCryptCallback encrypt_callback,
    OSCryptCallback decrypt_callback) {
  EncryptCallbackForTesting() = std::move(encrypt_callback);  // IN-TEST
  DecryptCallbackForTesting() = std::move(decrypt_callback);  // IN-TEST
}

BraveAccountEncryption::BraveAccountEncryption(
    const os_crypt_async::Encryptor& encryptor)
    : encryptor_(encryptor) {}

std::string BraveAccountEncryption::Encrypt(
    const std::string& plain_text) const {
  if (plain_text.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (const auto& callback = EncryptCallbackForTesting()) {
    CHECK_IS_TEST();
    if (!callback.Run(plain_text, &encrypted)) {
      return std::string();
    }
  } else {
    if (!encryptor_->EncryptString(plain_text, &encrypted)) {
      return std::string();
    }
  }

  return base::Base64Encode(encrypted);
}

std::string BraveAccountEncryption::Decrypt(const std::string& base64) const {
  if (base64.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!base::Base64Decode(base64, &encrypted)) {
    return std::string();
  }

  std::string plain_text;
  if (const auto& callback = DecryptCallbackForTesting()) {
    CHECK_IS_TEST();
    if (!callback.Run(encrypted, &plain_text)) {
      return std::string();
    }
  } else {
    if (!encryptor_->DecryptString(encrypted, &plain_text)) {
      return std::string();
    }
  }

  return plain_text;
}

// static
BraveAccountEncryption::OSCryptCallback&
BraveAccountEncryption::EncryptCallbackForTesting() {
  static base::NoDestructor<OSCryptCallback> callback;
  return *callback;
}

// static
BraveAccountEncryption::OSCryptCallback&
BraveAccountEncryption::DecryptCallbackForTesting() {
  static base::NoDestructor<OSCryptCallback> callback;
  return *callback;
}

}  // namespace brave_account
