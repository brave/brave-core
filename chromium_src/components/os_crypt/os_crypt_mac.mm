/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/keychain_password_mac.h"

#define KeychainPassword                                  \
  *KeychainPassword::service_name = "Brave Safe Storage"; \
  *KeychainPassword::account_name = "Brave";              \
  KeychainPassword

#include "../../../../components/os_crypt/os_crypt_mac.mm"
#undef KeychainPassword