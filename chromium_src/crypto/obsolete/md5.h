/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CRYPTO_OBSOLETE_MD5_H_
#define BRAVE_CHROMIUM_SRC_CRYPTO_OBSOLETE_MD5_H_

// In order to continue using MD5 (which has now moved to //crypto/obsolete) for
// our default protocol handler, we must mark our helper function as a friend of
// Md5. This is a standard pattern used by upstream code that wishes to continue
// using Md5.
#define KnownAnswer                                                    \
  KnownAnswer); friend std::array<uint8_t, crypto::obsolete::kMd5Size> \
  brave::Md5ForDefaultProtocolHandler(base::span<const uint8_t> data

#define BRAVE_CRYPTO_OBSOLETE_MD5                               \
  namespace brave {                                             \
  std::array<uint8_t, crypto::obsolete::kMd5Size>               \
  Md5ForDefaultProtocolHandler(base::span<const uint8_t> data); \
  }

#include <crypto/obsolete/md5.h>  // IWYU pragma: export

#undef BRAVE_CRYPTO_OBSOLETE_MD5
#undef KnownAnswer

#endif  // BRAVE_CHROMIUM_SRC_CRYPTO_OBSOLETE_MD5_H_
