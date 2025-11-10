// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_BASE_MEMORY_MEMORY_PRESSURE_LISTENER_H_
#define BRAVE_CHROMIUM_SRC_BASE_MEMORY_MEMORY_PRESSURE_LISTENER_H_

#define kMemoryReclaimerPressureListener \
  kRewardsDatabase = 1000, kAdsDatabase = 1001, kMemoryReclaimerPressureListener

#include <base/memory/memory_pressure_listener.h>  // IWYU pragma: export

#undef kMemoryReclaimerPressureListener

#endif  // BRAVE_CHROMIUM_SRC_BASE_MEMORY_MEMORY_PRESSURE_LISTENER_H_
