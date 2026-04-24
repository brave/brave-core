// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/os_crypt/sync/os_crypt_mocker_linux.h"

#include <memory>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/rand_util.h"
#include "brave/components/os_crypt/sync/key_storage_config_linux.h"
#include "brave/components/os_crypt/sync/os_crypt.h"

namespace {

std::unique_ptr<KeyStorageLinux> CreateNewMock() {
  return std::make_unique<OSCryptMockerLinux>();
}

}  // namespace

std::optional<std::string> OSCryptMockerLinux::GetKeyImpl() {
  return key_;
}

std::string* OSCryptMockerLinux::GetKeyPtr() {
  return &key_;
}

// static
void OSCryptMockerLinux::SetUp() {
  OSCrypt::UseMockKeyStorageForTesting(base::BindOnce(&CreateNewMock));
}

// static
void OSCryptMockerLinux::TearDown() {
  OSCrypt::UseMockKeyStorageForTesting(base::NullCallback());
  OSCrypt::ClearCacheForTesting();
}

bool OSCryptMockerLinux::Init() {
  key_ = "the_encryption_key";
  return true;
}
