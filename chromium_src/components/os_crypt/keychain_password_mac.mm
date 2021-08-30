/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/keychain_password_mac.h"

#include <memory>

#include "base/command_line.h"

#define BRAVE_KEYCHAIN_PASSWORD_GET_PASSWORD                                \
  std::unique_ptr<std::string> service_name, account_name;                  \
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess(); \
  if (command_line->HasSwitch("import-chrome")) {                           \
    service_name = std::make_unique<std::string>("Chrome Safe Storage");    \
    account_name = std::make_unique<std::string>("Chrome");                 \
  } else if (command_line->HasSwitch("import-chromium") ||                  \
             command_line->HasSwitch("import-brave")) {                     \
    service_name = std::make_unique<std::string>("Chromium Safe Storage");  \
    account_name = std::make_unique<std::string>("Chromium");               \
  } else {                                                                  \
    service_name = std::make_unique<std::string>(                           \
        ::KeychainPassword::GetServiceName().c_str());                      \
    account_name = std::make_unique<std::string>(                           \
        ::KeychainPassword::GetAccountName().c_str());                      \
  }

#define KeychainPassword KeychainPassword_ChromiumImpl
#include "../../../../components/os_crypt/keychain_password_mac.mm"
#undef KeychainPassword
#undef BRAVE_KEYCHAIN_PASSWORD_GET_PASSWORD

const char kBraveDefaultServiceName[] = "Brave Safe Storage";
const char kBraveDefaultAccountName[] = "Brave";

// static
KeychainPassword::KeychainNameType& KeychainPassword::GetServiceName() {
  static KeychainNameContainerType service_name(kBraveDefaultServiceName);
  return *service_name;
}

// static
KeychainPassword::KeychainNameType& KeychainPassword::GetAccountName() {
  static KeychainNameContainerType account_name(kBraveDefaultAccountName);
  return *account_name;
}

KeychainPassword::KeychainPassword(const AppleKeychain& keychain)
    : KeychainPassword_ChromiumImpl(keychain) {}

KeychainPassword::~KeychainPassword() = default;
