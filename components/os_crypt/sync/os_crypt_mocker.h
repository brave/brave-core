// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_MOCKER_H_
#define BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_MOCKER_H_

#include <string>

#include "build/build_config.h"

// Handles the mocking of OSCrypt, such that it does not reach system level
// services.
class OSCryptMocker {
 public:
  OSCryptMocker(const OSCryptMocker&) = delete;
  OSCryptMocker& operator=(const OSCryptMocker&) = delete;

  // Inject mocking into OSCrypt.
  static void SetUp();

  // Obtain the raw encryption key from OSCrypt. This is used to e.g. initialize
  // the mock key in another process.
  static std::string GetRawEncryptionKey();

#if BUILDFLAG(IS_APPLE)
  // Pretend that backend for storing keys is unavailable.
  static void SetBackendLocked(bool locked);
#endif

#if BUILDFLAG(IS_WIN)
  // Store data using the older DPAPI interface rather than session key.
  static void SetLegacyEncryption(bool legacy);

  // Reset OSCrypt so it can be initialized again with a new profile/key.
  static void ResetState();
#endif

  // Restore OSCrypt to its real behaviour.
  static void TearDown();
};

#endif  // BRAVE_COMPONENTS_OS_CRYPT_SYNC_OS_CRYPT_MOCKER_H_
