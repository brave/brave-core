// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_TRACKER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_TRACKER_H_

#include "base/functional/callback.h"

#define ForcePulseForTest                                                      \
  DeleteDeviceInfo(const std::string& client_id, base::OnceClosure callback) { \
  }                                                                            \
  virtual std::vector<DeviceInfo> GetAllBraveDeviceInfo() const;               \
  /* Sets the Brave-only display label of any device in the sync chain, */     \
  /* clearing it when `display_label` is empty. */                             \
  virtual void SetDeviceDisplayLabel(const std::string& client_id,             \
                                     const std::string& display_label) {}      \
  virtual void ForcePulseForTest

#include <components/sync_device_info/device_info_tracker.h>  // IWYU pragma: export

#undef ForcePulseForTest

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_TRACKER_H_
