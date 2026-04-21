// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_MOCKER_LINUX_H_
#define BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_MOCKER_LINUX_H_

#include <optional>
#include <string>

#include "brave/components/os_crypt/sync/key_storage_linux.h"

// Holds and serves a password from memory.
class OSCryptMockerLinux : public KeyStorageLinux {
 public:
  OSCryptMockerLinux() = default;

  OSCryptMockerLinux(const OSCryptMockerLinux&) = delete;
  OSCryptMockerLinux& operator=(const OSCryptMockerLinux&) = delete;

  ~OSCryptMockerLinux() override = default;

  // Get a pointer to the stored password. OSCryptMockerLinux owns the pointer.
  std::string* GetKeyPtr();

  // Inject the mocking scheme into OSCrypt.
  static void SetUp();

  // Restore OSCrypt to its real behaviour.
  static void TearDown();

 protected:
  // KeyStorageLinux
  bool Init() override;
  std::optional<std::string> GetKeyImpl() override;

 private:
  std::string key_;
};

#endif  // BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_MOCKER_LINUX_H_
