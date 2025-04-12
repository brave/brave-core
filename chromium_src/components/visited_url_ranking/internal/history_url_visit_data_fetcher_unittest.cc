/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync_device_info/brave_device_info.h"
#include "components/history/core/browser/history_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class BraveDeviceInfoTracker : public syncer::DeviceInfoTracker {
 public:
  MOCK_CONST_METHOD0(GetAllBraveDeviceInfo,
                     std::vector<std::unique_ptr<BraveDeviceInfo>>());
};

}  // namespace syncer

#define DeviceInfoTracker BraveDeviceInfoTracker
#include "src/components/visited_url_ranking/internal/history_url_visit_data_fetcher_unittest.cc"
#undef DeviceInfoTracker
