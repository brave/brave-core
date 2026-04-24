// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/os_crypt/sync/os_crypt_mocker.h"

#include <memory>

#include "brave/components/os_crypt/sync/os_crypt.h"
#include "crypto/apple/fake_keychain_v2.h"
#include "crypto/apple/keychain_v2.h"

// static
void OSCryptMocker::SetUp() {
  OSCrypt::SetKeychainForTesting(
      std::make_unique<crypto::apple::FakeKeychainV2>("test-access-group"));
}

// static
void OSCryptMocker::SetBackendLocked(bool locked) {
  if (locked) {
    OSCrypt::SetKeychainForTesting(OSCrypt::MockLockedKeychain());
  } else {
    OSCrypt::SetKeychainForTesting(
        std::make_unique<crypto::apple::FakeKeychainV2>("test-access-group"));
  }
}

// static
void OSCryptMocker::TearDown() {
  OSCrypt::SetKeychainForTesting(
      std::unique_ptr<crypto::apple::FakeKeychainV2>());
}
