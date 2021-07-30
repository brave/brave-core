/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/key_storage_keyring.h"

#include <string>

#include "base/command_line.h"
#include "components/os_crypt/keyring_util_linux.h"

namespace {
const char* GetApplicationName();
void Dummy(const GnomeKeyringPasswordSchema*,
           gchar**,
           const char*,
           const char*,
           void*) {}
}  // namespace

#define BRAVE_KEY_STORAGE_KEYRING_GET_KEY_IMPL                          \
  &kSchema, &password_c, "application", GetApplicationName(), nullptr); \
  if (false) Dummy(

#include "../../../../components/os_crypt/key_storage_keyring.cc"
#undef BRAVE_KEY_STORAGE_KEYRING_GET_KEY_IMPL

namespace {

const char* GetApplicationName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-chrome")) {
    return "chrome";
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    return "chromium";
  } else {
    return "brave";
  }
}

}  // namespace
