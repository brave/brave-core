/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OS_CRYPT_KEYCHAIN_PASSWORD_MAC_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OS_CRYPT_KEYCHAIN_PASSWORD_MAC_H_

#define KeychainPassword KeychainPassword_ChromiumImpl
#include "../../../../components/os_crypt/keychain_password_mac.h"
#undef KeychainPassword

class COMPONENT_EXPORT(OS_CRYPT) KeychainPassword
    : public KeychainPassword_ChromiumImpl {
 public:
  KeychainPassword(const crypto::AppleKeychain& keychain);
  ~KeychainPassword();

  // The service and account names used in Brave's Safe Storage keychain item.
  // Hides base implementation
  static COMPONENT_EXPORT(OS_CRYPT) KeychainNameType& GetServiceName();
  static COMPONENT_EXPORT(OS_CRYPT) KeychainNameType& GetAccountName();

 private:
  KeychainPassword(const KeychainPassword&) = delete;
  KeychainPassword& operator=(const KeychainPassword&) = delete;
};

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OS_CRYPT_KEYCHAIN_PASSWORD_MAC_H_
