/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/key_storage_linux.h"

#define BRAVE_KEY_STORAGE_LINUX                             \
  const char KeyStorageLinux::kFolderName[] = "Brave Keys"; \
  const char KeyStorageLinux::kKey[] = "Brave Safe Storage";

#define BRAVE_KEY_STORAGE_LINUX_CREATE_SERVICE_INTERNAL                       \
  static const base::NoDestructor<std::string> kDefaultApplicationName("brave");

#include "../../../../components/os_crypt/key_storage_linux.cc"
#undef BRAVE_KEY_STORAGE_LINUX_CREATE_SERVICE_INTERNAL
#undef BRAVE_KEY_STORAGE_LINUX
