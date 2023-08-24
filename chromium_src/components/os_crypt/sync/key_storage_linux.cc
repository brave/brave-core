/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/sync/key_storage_linux.h"

#define BRAVE_KEY_STORAGE_LINUX                             \
  const char KeyStorageLinux::kFolderName[] = "Brave Keys"; \
  const char KeyStorageLinux::kKey[] = "Brave Safe Storage";

#define BRAVE_KEY_STORAGE_LINUX_CREATE_SERVICE                          \
  if (use_backend && config.store.empty() &&                            \
      selected_backend == os_crypt::SelectedLinuxBackend::BASIC_TEXT) { \
    VLOG(1) << "Trying to find a working backend manually.";            \
    for (const auto fallback_backend : {                                \
             os_crypt::SelectedLinuxBackend::GNOME_LIBSECRET,           \
             os_crypt::SelectedLinuxBackend::KWALLET6,                  \
             os_crypt::SelectedLinuxBackend::KWALLET5,                  \
             os_crypt::SelectedLinuxBackend::KWALLET,                   \
         }) {                                                           \
      key_storage = CreateServiceInternal(fallback_backend, config);    \
      if (key_storage) {                                                \
        VLOG(1) << "Successfully found a working backend: "             \
                << SelectedLinuxBackendToString(fallback_backend);      \
        break;                                                          \
      }                                                                 \
    }                                                                   \
    if (!key_storage) {                                                 \
      VLOG(1) << "Did not find a working backend.";                     \
    }                                                                   \
  }

// clang-format off
#define BRAVE_KEY_STORAGE_LINUX_CREATE_SERVICE_INTERNAL                       \
  static const base::NoDestructor<std::string> kDefaultApplicationName("brave");
// clang-format on

#include "src/components/os_crypt/sync/key_storage_linux.cc"
#undef BRAVE_KEY_STORAGE_LINUX_CREATE_SERVICE_INTERNAL
#undef BRAVE_KEY_STORAGE_LINUX_CREATE_SERVICE
#undef BRAVE_KEY_STORAGE_LINUX
