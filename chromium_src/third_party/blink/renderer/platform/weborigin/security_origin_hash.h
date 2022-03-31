/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_ORIGIN_HASH_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_ORIGIN_HASH_H_

#define safe_to_compare_to_empty_or_deleted                              \
  unused = false;                                                        \
  static const base::UnguessableToken GetNonceForEphemeralStorageKeying( \
      const SecurityOrigin* origin) {                                    \
    CHECK(origin->IsOpaque());                                           \
    return *origin->GetNonceForSerialization();                          \
  }                                                                      \
  static const base::UnguessableToken GetNonceForEphemeralStorageKeying( \
      const scoped_refptr<const SecurityOrigin>& origin) {               \
    return GetNonceForEphemeralStorageKeying(origin.get());              \
  }                                                                      \
  static const bool safe_to_compare_to_empty_or_deleted

#include "src/third_party/blink/renderer/platform/weborigin/security_origin_hash.h"

#undef safe_to_compare_to_empty_or_deleted

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_ORIGIN_HASH_H_
