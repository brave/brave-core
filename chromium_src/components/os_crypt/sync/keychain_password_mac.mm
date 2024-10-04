/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/sync/keychain_password_mac.h"

#include <utility>

#include "base/command_line.h"

namespace {

constexpr char kBraveDefaultServiceName[] = "Brave Safe Storage";
constexpr char kBraveDefaultAccountName[] = "Brave";

KeychainPassword::KeychainNameType& GetBraveServiceName();
KeychainPassword::KeychainNameType& GetBraveAccountName();

}  // namespace

#define BRAVE_GET_SERVICE_NAME return GetBraveServiceName();
#define BRAVE_GET_ACCOUNT_NAME return GetBraveAccountName();
#include "src/components/os_crypt/sync/keychain_password_mac.mm"
#undef BRAVE_GET_SERVICE_NAME
#undef BRAVE_GET_ACCOUNT_NAME

namespace {

std::pair<std::string, std::string> GetServiceAndAccountName() {
  std::string service_name, account_name;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-edge")) {
    service_name = std::string("Microsoft Edge Safe Storage");
    account_name = std::string("Microsoft Edge");
  } else if (command_line->HasSwitch("import-yandex")) {
    service_name = std::string("Yandex Safe Storage");
    account_name = std::string("Yandex");
  } else if (command_line->HasSwitch("import-whale")) {
    service_name = std::string("Whale Safe Storage");
    account_name = std::string("Whale");
  } else if (command_line->HasSwitch("import-chrome")) {
    service_name = std::string("Chrome Safe Storage");
    account_name = std::string("Chrome");
  } else if (command_line->HasSwitch("import-vivaldi")) {
    service_name = std::string("Vivaldi Safe Storage");
    account_name = std::string("Vivaldi");
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    service_name = std::string("Chromium Safe Storage");
    account_name = std::string("Chromium");
  } else if (command_line->HasSwitch("import-opera")) {
    service_name = std::string("Opera Safe Storage");
    account_name = std::string("Opera");
  } else {
    service_name = std::string(kBraveDefaultServiceName);
    account_name = std::string(kBraveDefaultAccountName);
  }
  return std::make_pair(service_name, account_name);
}

KeychainPassword::KeychainNameType& GetBraveServiceName() {
  static KeychainNameContainerType service_name(
      GetServiceAndAccountName().first);
  return *service_name;
}

KeychainPassword::KeychainNameType& GetBraveAccountName() {
  static KeychainNameContainerType account_name(
      GetServiceAndAccountName().second);
  return *account_name;
}

}  // namespace
