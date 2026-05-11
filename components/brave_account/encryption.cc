/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/encryption.h"

#include <utility>

#include "base/base64.h"
#include "base/check_is_test.h"
#include "base/no_destructor.h"

namespace brave_account::internal {

namespace {

OSCryptCallback& EncryptCallbackForTesting() {
  static base::NoDestructor<OSCryptCallback> callback;
  return *callback;
}

OSCryptCallback& DecryptCallbackForTesting() {
  static base::NoDestructor<OSCryptCallback> callback;
  return *callback;
}

}  // namespace

void SetOSCryptCallbacksForTesting(  // IN-TEST
    OSCryptCallback encrypt_callback,
    OSCryptCallback decrypt_callback) {
  EncryptCallbackForTesting() = std::move(encrypt_callback);  // IN-TEST
  DecryptCallbackForTesting() = std::move(decrypt_callback);  // IN-TEST
}

std::string Encrypt(const os_crypt_async::Encryptor& encryptor,
                    const std::string& plain_text) {
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
    if (!encryptor.EncryptString(plain_text, &encrypted)) {
      return std::string();
    }
  }

  return base::Base64Encode(encrypted);
}

std::string Decrypt(const os_crypt_async::Encryptor& encryptor,
                    const std::string& base64) {
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
    if (!encryptor.DecryptString(encrypted, &plain_text)) {
      return std::string();
    }
  }

  return plain_text;
}

}  // namespace brave_account::internal
