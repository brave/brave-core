/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_SCHEMEFUL_SITE_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_SCHEMEFUL_SITE_H_

#define NetworkIsolationKey \
  NetworkIsolationKey;      \
  friend class HSTSPartitionHashHelper

#include "src/net/base/schemeful_site.h"

#undef NetworkIsolationKey

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_SCHEMEFUL_SITE_H_
