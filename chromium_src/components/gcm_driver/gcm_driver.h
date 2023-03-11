/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_GCM_DRIVER_GCM_DRIVER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_GCM_DRIVER_GCM_DRIVER_H_

#define GetGCMStatistics      \
  SetEnabled(bool enabled) {} \
  virtual void GetGCMStatistics

#include "src/components/gcm_driver/gcm_driver.h"  // IWYU pragma: export

#undef GetGCMStatistics

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_GCM_DRIVER_GCM_DRIVER_H_
