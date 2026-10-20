// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/sync_trust_source.h"

#include "base/test/task_environment.h"
#include "components/sync_device_info/fake_device_info_sync_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

class CountingObserver : public TrustSource::Observer {
 public:
  void OnTrustChanged() override { ++changes; }

  int changes = 0;
};

}  // namespace

TEST(BraveTransferSyncTrustSourceTest, DeviceTypes) {
  using FormFactor = syncer::DeviceInfo::FormFactor;
  EXPECT_EQ(DeviceTypeFromFormFactor(FormFactor::kDesktop),
            mojom::DeviceType::kDesktop);
  EXPECT_EQ(DeviceTypeFromFormFactor(FormFactor::kPhone),
            mojom::DeviceType::kPhone);
  EXPECT_EQ(DeviceTypeFromFormFactor(FormFactor::kTablet),
            mojom::DeviceType::kTablet);
  EXPECT_EQ(DeviceTypeFromFormFactor(FormFactor::kTv),
            mojom::DeviceType::kUnknown);
  EXPECT_EQ(DeviceTypeFromFormFactor(FormFactor::kUnknown),
            mojom::DeviceType::kUnknown);
}

TEST(BraveTransferSyncTrustSourceTest, InactiveWithoutSync) {
  base::test::TaskEnvironment task_environment;
  syncer::FakeDeviceInfoSyncService device_info_service;
  SyncTrustSource trust_source(/*sync_service=*/nullptr, &device_info_service);
  CountingObserver observer;
  trust_source.AddObserver(&observer);

  EXPECT_FALSE(trust_source.IsSyncActive());
  EXPECT_FALSE(trust_source.GetChainKeys());
  EXPECT_TRUE(trust_source.GetLocalDeviceGuid().empty());
  EXPECT_TRUE(trust_source.GetLocalDeviceName().empty());
  EXPECT_TRUE(trust_source.GetChainDevices().empty());

  device_info_service.GetDeviceInfoTracker()->SetLocalCacheGuid("local");
  EXPECT_EQ(observer.changes, 0);

  trust_source.RemoveObserver(&observer);
}

}  // namespace brave_transfer
