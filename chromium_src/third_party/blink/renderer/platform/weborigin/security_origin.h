/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_ORIGIN_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_ORIGIN_H_

#define SerializesAsNull                                                 \
  SerializesAsNull_Unused();                                             \
  static const base::UnguessableToken GetNonceForEphemeralStorageKeying( \
      const blink::SecurityOrigin* origin) {                             \
    CHECK(origin->IsOpaque());                                           \
    return *origin->GetNonceForSerialization();                          \
  }                                                                      \
  static const base::UnguessableToken GetNonceForEphemeralStorageKeying( \
      const scoped_refptr<const blink::SecurityOrigin>& origin) {        \
    return GetNonceForEphemeralStorageKeying(origin.get());              \
  }                                                                      \
  bool SerializesAsNull

#include "src/third_party/blink/renderer/platform/weborigin/security_origin.h"  // IWYU pragma: export

#undef SerializesAsNull

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_ORIGIN_H_
