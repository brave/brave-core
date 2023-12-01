/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>

#include "base/command_line.h"
#include "components/os_crypt/sync/key_storage_kwallet.h"

namespace {
void Dummy(const int handle,
           const std::string& folder_name,
           const std::string& app_name,
           bool* has_folder_ptr) {}
}  // namespace

#define BRAVE_KEY_STORAGE_KWALLET_GET_KEY_IMPL                            \
  if (kwallet_dbus_->ReadPassword(handle_, GetFolderName(), GetKeyName(), \
                                  app_name_,                              \
                                  &password) != KWalletDBus::SUCCESS) {   \
    return std::nullopt;                                                  \
    /* NOLINTNEXTLINE */                                                  \
  } else if (false)

#define BRAVE_KEY_STORAGE_KWALLET_INIT_FOLDER        \
  handle_, GetFolderName(), app_name_, &has_folder); \
  if (false) Dummy(

#include "src/components/os_crypt/sync/key_storage_kwallet.cc"
#undef BRAVE_KEY_STORAGE_KWALLET_INIT_FOLDER
#undef BRAVE_KEY_STORAGE_KWALLET_GET_KEY_IMPL

const char* KeyStorageKWallet::GetFolderName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-chrome")) {
    return "Chrome Keys";
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    return "Chromium Keys";
  } else {
    return KeyStorageLinux::kFolderName;
  }
}

const char* KeyStorageKWallet::GetKeyName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-chrome")) {
    return "Chrome Safe Storage";
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    return "Chromium Safe Storage";
  } else {
    return KeyStorageLinux::kKey;
  }
}
