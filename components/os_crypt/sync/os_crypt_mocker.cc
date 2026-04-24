// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/os_crypt/sync/os_crypt_mocker.h"

#include <memory>

#include "brave/components/os_crypt/sync/os_crypt.h"
#include "build/build_config.h"

#if defined(USE_LIBSECRET) || defined(USE_KWALLET)
#include "brave/components/os_crypt/sync/os_crypt_mocker_linux.h"
#endif

// static
#if !BUILDFLAG(IS_APPLE)
void OSCryptMocker::SetUp() {
#if defined(USE_LIBSECRET) || defined(USE_KWALLET)
  OSCryptMockerLinux::SetUp();
#elif BUILDFLAG(IS_WIN)
  OSCrypt::UseMockKeyForTesting(true);
#endif
}
#endif

// static
std::string OSCryptMocker::GetRawEncryptionKey() {
  return OSCrypt::GetRawEncryptionKey();
}

#if BUILDFLAG(IS_WIN)
// static
void OSCryptMocker::SetLegacyEncryption(bool legacy) {
  OSCrypt::SetLegacyEncryptionForTesting(legacy);
}

void OSCryptMocker::ResetState() {
  OSCrypt::ResetStateForTesting();
}

#endif

// static
#if !BUILDFLAG(IS_APPLE)
void OSCryptMocker::TearDown() {
#if defined(USE_LIBSECRET) || defined(USE_KWALLET)
  OSCryptMockerLinux::TearDown();
#elif BUILDFLAG(IS_WIN)
  OSCrypt::UseMockKeyForTesting(false);
#endif
}
#endif
