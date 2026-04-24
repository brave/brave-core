// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OS_CRYPT_SYNC_KEY_STORAGE_LIBSECRET_H_
#define BRAVE_COMPONENTS_OS_CRYPT_SYNC_KEY_STORAGE_LIBSECRET_H_

#include <optional>
#include <string>

#include "base/component_export.h"
#include "brave/components/os_crypt/sync/key_storage_linux.h"

// Specialisation of KeyStorageLinux that uses Libsecret.
class COMPONENT_EXPORT(OS_CRYPT) KeyStorageLibsecret : public KeyStorageLinux {
 public:
  explicit KeyStorageLibsecret(std::string application_name);

  KeyStorageLibsecret(const KeyStorageLibsecret&) = delete;
  KeyStorageLibsecret& operator=(const KeyStorageLibsecret&) = delete;

  ~KeyStorageLibsecret() override = default;

 protected:
  // KeyStorageLinux
  bool Init() override;
  std::optional<std::string> GetKeyImpl() override;

 private:
  std::optional<std::string> AddRandomPasswordInLibsecret();

  const std::string application_name_;
};

#endif  // BRAVE_COMPONENTS_OS_CRYPT_SYNC_KEY_STORAGE_LIBSECRET_H_
