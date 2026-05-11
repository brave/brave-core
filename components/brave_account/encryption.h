/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENCRYPTION_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENCRYPTION_H_

#include <string>

#include "base/functional/callback.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace brave_account::internal {

using OSCryptCallback =
    base::RepeatingCallback<bool(const std::string&, std::string*)>;

void SetOSCryptCallbacksForTesting(OSCryptCallback encrypt_callback,
                                   OSCryptCallback decrypt_callback);

std::string Encrypt(const os_crypt_async::Encryptor& encryptor,
                    const std::string& plain_text);

std::string Decrypt(const os_crypt_async::Encryptor& encryptor,
                    const std::string& base64);

}  // namespace brave_account::internal

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENCRYPTION_H_
