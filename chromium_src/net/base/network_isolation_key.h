/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_NETWORK_ISOLATION_KEY_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_NETWORK_ISOLATION_KEY_H_

#define GetTopFrameOrigin             \
  GetEffectiveTopFrameOrigin() const; \
  const base::Optional<url::Origin>& GetTopFrameOrigin

#include "../../../../net/base/network_isolation_key.h"  // NOLINT

#undef GetTopFrameOrigin

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_NETWORK_ISOLATION_KEY_H_
