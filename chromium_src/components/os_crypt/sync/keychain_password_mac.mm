/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/sync/keychain_password_mac.h"

#include <utility>

#include "base/command_line.h"

namespace {

KeychainPassword::KeychainNameType& GetBraveServiceName();
KeychainPassword::KeychainNameType& GetBraveAccountName();

}  // namespace

#define BRAVE_GET_SERVICE_NAME return GetBraveServiceName();
#define BRAVE_GET_ACCOUNT_NAME return GetBraveAccountName();
#include "src/components/os_crypt/sync/keychain_password_mac.mm"
#undef BRAVE_GET_SERVICE_NAME
#undef BRAVE_GET_ACCOUNT_NAME

namespace {

KeychainPassword::KeychainNameType& GetBraveServiceName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-edge")) {
    static KeychainNameContainerType kEdgeServiceName(
        "Microsoft Edge Safe Storage");
    return *kEdgeServiceName;
  } else if (command_line->HasSwitch("import-yandex")) {
    static KeychainNameContainerType kYandexServiceName("Yandex Safe Storage");
    return *kYandexServiceName;
  } else if (command_line->HasSwitch("import-whale")) {
    static KeychainNameContainerType kWhaleServiceName("Whale Safe Storage");
    return *kWhaleServiceName;
  } else if (command_line->HasSwitch("import-chrome")) {
    static KeychainNameContainerType kChromeServiceName("Chrome Safe Storage");
    return *kChromeServiceName;
  } else if (command_line->HasSwitch("import-vivaldi")) {
    static KeychainNameContainerType kVivaldiServiceName(
        "Vivaldi Safe Storage");
    return *kVivaldiServiceName;
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    static KeychainNameContainerType kChromiumServiceName(
        "Chromium Safe Storage");
    return *kChromiumServiceName;
  } else if (command_line->HasSwitch("import-opera")) {
    static KeychainNameContainerType kOperaServiceName("Opera Safe Storage");
    return *kOperaServiceName;
  } else {
    static KeychainNameContainerType kBraveDefaultServiceName(
        "Brave Safe Storage");
    return *kBraveDefaultServiceName;
  }
}

KeychainPassword::KeychainNameType& GetBraveAccountName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-edge")) {
    static KeychainNameContainerType kEdgeAccountName("Microsoft Edge");
    return *kEdgeAccountName;
  } else if (command_line->HasSwitch("import-yandex")) {
    static KeychainNameContainerType kYandexAccountName("Yandex");
    return *kYandexAccountName;
  } else if (command_line->HasSwitch("import-whale")) {
    static KeychainNameContainerType kWhaleAccountName("Whale");
    return *kWhaleAccountName;
  } else if (command_line->HasSwitch("import-chrome")) {
    static KeychainNameContainerType kChromeAccountName("Chrome");
    return *kChromeAccountName;
  } else if (command_line->HasSwitch("import-vivaldi")) {
    static KeychainNameContainerType kVivaldiAccountName("Vivaldi");
    return *kVivaldiAccountName;
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    static KeychainNameContainerType kChromiumAccountName("Chromium");
    return *kChromiumAccountName;
  } else if (command_line->HasSwitch("import-opera")) {
    static KeychainNameContainerType kOperaAccountName("Opera");
    return *kOperaAccountName;
  } else {
    static KeychainNameContainerType kBraveDefaultAccountName("Brave");
    return *kBraveDefaultAccountName;
  }
}

}  // namespace
