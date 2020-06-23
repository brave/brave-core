// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "../../../../components/sync_device_info/device_info_sync_bridge_unittest.cc"

namespace syncer {
namespace {

TEST_F(DeviceInfoSyncBridgeTest, LocalDelete) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(1, change_count());
  ASSERT_FALSE(ReadAllFromStore().empty());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1, base::Time::Now());
  auto error = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                          EntityAddList({specifics}));

  ASSERT_FALSE(error);
  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(2, change_count());

  const std::string kLocalGuid = CacheGuidForSuffix(kLocalSuffix);
  ON_CALL(*processor(), IsEntityUnsynced(kLocalGuid))
      .WillByDefault(Return(false));
  EXPECT_CALL(*processor(), Delete(kLocalGuid, _)).Times(1);

  bool deleted_device_info_sent = false;
  base::RunLoop loop;
  bridge()->DeleteDeviceInfo(
      kLocalGuid, base::BindOnce(
                      [](base::RunLoop* loop, bool* deleted_device_info_sent) {
                        *deleted_device_info_sent = true;
                        loop->Quit();
                      },
                      &loop, &deleted_device_info_sent));
  loop.Run();

  EXPECT_TRUE(deleted_device_info_sent);
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(3, change_count());
  EXPECT_THAT(ReadAllFromStore(),
              UnorderedElementsAre(Pair(specifics.cache_guid(), _)));
}

TEST_F(DeviceInfoSyncBridgeTest, RemoteDelete) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(1, change_count());
  ASSERT_FALSE(ReadAllFromStore().empty());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1, base::Time::Now());
  auto error = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                          EntityAddList({specifics}));

  ASSERT_FALSE(error);
  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(2, change_count());

  const std::string kLocalGuid = CacheGuidForSuffix(kLocalSuffix);
  ON_CALL(*processor(), IsEntityUnsynced(specifics.cache_guid()))
      .WillByDefault(Return(false));
  EXPECT_CALL(*processor(), Delete(specifics.cache_guid(), _)).Times(1);

  bool deleted_device_info_sent = false;
  base::RunLoop loop;
  bridge()->DeleteDeviceInfo(
      specifics.cache_guid(),
      base::BindOnce(
          [](base::RunLoop* loop, bool* deleted_device_info_sent) {
            *deleted_device_info_sent = true;
            loop->Quit();
          },
          &loop, &deleted_device_info_sent));
  loop.Run();

  EXPECT_TRUE(deleted_device_info_sent);
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(3, change_count());
  EXPECT_THAT(ReadAllFromStore(), UnorderedElementsAre(Pair(kLocalGuid, _)));
}

}  // namespace
}  // namespace syncer
