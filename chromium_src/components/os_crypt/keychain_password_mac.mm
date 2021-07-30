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
        ::KeychainPassword::service_name->c_str());                         \
    account_name = std::make_unique<std::string>(                           \
        ::KeychainPassword::account_name->c_str());                         \
  }

#include "../../../../components/os_crypt/keychain_password_mac.mm"
#undef BRAVE_KEYCHAIN_PASSWORD_GET_PASSWORD