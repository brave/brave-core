/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_URL_ORIGIN_H_
#define BRAVE_CHROMIUM_SRC_URL_ORIGIN_H_

namespace net {
class EphemeralStorageOriginUtils;
}  // namespace net

#define GetTupleOrPrecursorTupleIfOpaque   \
  NotUsed() const;                         \
  friend net::EphemeralStorageOriginUtils; \
  const SchemeHostPort& GetTupleOrPrecursorTupleIfOpaque

#include "src/url/origin.h"

#undef GetTupleOrPrecursorTupleIfOpaque

#endif  // BRAVE_CHROMIUM_SRC_URL_ORIGIN_H_
