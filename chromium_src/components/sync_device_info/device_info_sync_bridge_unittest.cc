// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <optional>
#include <string_view>

#include "base/system/sys_info.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace test {
namespace {

// This trick substitutes TaskEnvironment to make the test
// LocalDeleteWhenOffline avoid wait 6 sec for real
class TaskEnvironmentOptionalMockTime : public TaskEnvironment {
 public:
  TaskEnvironmentOptionalMockTime()
      : TaskEnvironment(
            IsMockTimedTest(
                testing::UnitTest::GetInstance()->current_test_info()->name())
                ? TimeSource::MOCK_TIME
                : TimeSource::DEFAULT) {}

  static bool IsMockTimedTest(std::string_view test_name) {
    return test_name == "LocalDeleteWhenOffline";
  }
};

}  // namespace
}  // namespace test
}  // namespace base

#define TaskEnvironment TaskEnvironmentOptionalMockTime
#define ShouldReuploadOnceAfterLocalDeviceInfoTombstone \
  DISABLED_ShouldReuploadOnceAfterLocalDeviceInfoTombstone

#include "src/components/sync_device_info/device_info_sync_bridge_unittest.cc"

#undef TaskEnvironment
#undef ShouldReuploadOnceAfterLocalDeviceInfoTombstone

namespace syncer {
namespace {

TEST_F(DeviceInfoSyncBridgeTest, LocalDelete) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(1, change_count());
  ASSERT_FALSE(ReadAllFromStore().empty());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1, base::Time::Now());
  auto error = bridge()->ApplyIncrementalSyncChanges(
      bridge()->CreateMetadataChangeList(), EntityAddList({specifics}));

  ASSERT_FALSE(error);
  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(2, change_count());

  const std::string kLocalGuid = CacheGuidForSuffix(kLocalSuffix);
  ON_CALL(*processor(), IsEntityUnsynced(kLocalGuid))
      .WillByDefault(Return(false));
  EXPECT_CALL(*processor(), Delete(kLocalGuid, _, _)).Times(1);

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
  auto error = bridge()->ApplyIncrementalSyncChanges(
      bridge()->CreateMetadataChangeList(), EntityAddList({specifics}));

  ASSERT_FALSE(error);
  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(2, change_count());

  const std::string kLocalGuid = CacheGuidForSuffix(kLocalSuffix);
  ON_CALL(*processor(), IsEntityUnsynced(specifics.cache_guid()))
      .WillByDefault(Return(false));
  EXPECT_CALL(*processor(), Delete(specifics.cache_guid(), _, _)).Times(1);

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

TEST_F(DeviceInfoSyncBridgeTest, LocalDeleteWhenOffline) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(1, change_count());
  ASSERT_FALSE(ReadAllFromStore().empty());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1, base::Time::Now());
  auto error = bridge()->ApplyIncrementalSyncChanges(
      bridge()->CreateMetadataChangeList(), EntityAddList({specifics}));

  ASSERT_FALSE(error);
  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(2, change_count());

  const std::string kLocalGuid = CacheGuidForSuffix(kLocalSuffix);
  ON_CALL(*processor(), IsEntityUnsynced(kLocalGuid))
      .WillByDefault(Return(true));
  // The statement below means that DeviceInfoSyncBridge::OnDeviceInfoDeleted
  // 5 times did check of IsEntityUnsynced for the entity being deleted
  EXPECT_CALL(*processor(), IsEntityUnsynced).Times(5);
  EXPECT_CALL(*processor(), Delete(kLocalGuid, _, _)).Times(1);

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

TEST_F(DeviceInfoSyncBridgeTest,
       BraveApplyIncrementalSyncChangesForLocalDelete) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1, change_count());

  const DeviceInfoSpecifics specifics = CreateLocalDeviceSpecifics();
  syncer::EntityChangeList entity_change_list;
  entity_change_list.push_back(
      EntityChange::CreateDelete(specifics.cache_guid()));
  auto error_on_delete = bridge()->ApplyIncrementalSyncChanges(
      bridge()->CreateMetadataChangeList(), std::move(entity_change_list));
  EXPECT_FALSE(error_on_delete);
  EXPECT_EQ(2, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest,
       BraveShouldReuploadAfterLocalDeviceInfoTombstone) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1u, bridge()->GetAllDeviceInfo().size());

  EntityChangeList changes;
  changes.push_back(
      EntityChange::CreateDelete(CacheGuidForSuffix(kLocalSuffix)));

  // An incoming deletion for the local device info should not cause a reupload
  EXPECT_CALL(*processor(), Put(CacheGuidForSuffix(kLocalSuffix), _, _))
      .Times(0);
  std::optional<ModelError> error = bridge()->ApplyIncrementalSyncChanges(
      bridge()->CreateMetadataChangeList(), std::move(changes));
  ASSERT_FALSE(error);

  // The local device info should no longer exist.
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
}

TEST_F(DeviceInfoSyncBridgeTest, BraveExpireOldEntriesUponStartup) {
  InitializeAndMergeInitialData(SyncMode::kFull);
  ASSERT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(1, change_count());
  ASSERT_FALSE(ReadAllFromStore().empty());

  const DeviceInfoSpecifics specifics_old =
      CreateSpecifics(2, base::Time::Now() - base::Days(57));

  auto error = bridge()->ApplyIncrementalSyncChanges(
      bridge()->CreateMetadataChangeList(), EntityAddList({specifics_old}));

  ASSERT_FALSE(error);
  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  ASSERT_EQ(2, change_count());

  // Reloading from storage should not expire the old entity
  RestartBridge();
  EXPECT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  // Make sure this is well persisted to the DB store.
  EXPECT_THAT(ReadAllFromStore(),
              UnorderedElementsAre(
                  Pair(local_device()->GetLocalDeviceInfo()->guid(), _),
                  Pair(specifics_old.cache_guid(), _)));
}

TEST_F(DeviceInfoSyncBridgeTest, BraveResetsProgressMarkerOnce) {
  const DeviceInfoSpecifics specifics = CreateLocalDeviceSpecifics();
  DataTypeState data_type_state = StateWithEncryption("ekn");
  data_type_state.mutable_progress_marker()->set_token("ABC");
  WriteToStoreWithMetadata({specifics}, data_type_state);

  {
    base::RunLoop run_loop;
    InitializeBridge();

    // Wait until the metadata is loaded.
    EXPECT_CALL(*processor(), IsTrackingMetadata).WillOnce(Return(true));
    EXPECT_CALL(*processor(), ModelReadyToSync)
        .WillOnce([&run_loop](std::unique_ptr<MetadataBatch> batch) {
          // When model is loaded for the first time and the progress token
          // was set then the token should be reset
          EXPECT_TRUE(batch->GetDataTypeState().has_progress_marker());
          EXPECT_FALSE(batch->GetDataTypeState().progress_marker().has_token());
          run_loop.Quit();
        });

    run_loop.Run();
  }

  PumpAndShutdown();
  {
    base::RunLoop run_loop;
    InitializeBridge();
    EXPECT_CALL(*processor(), IsTrackingMetadata).WillOnce(Return(true));
    EXPECT_CALL(*processor(), ModelReadyToSync)
        .WillOnce([&run_loop](std::unique_ptr<MetadataBatch> batch) {
          // When the progress token already was reset, then do not reset it
          // again
          EXPECT_TRUE(batch->GetDataTypeState().has_progress_marker());
          EXPECT_TRUE(batch->GetDataTypeState().progress_marker().has_token());
          run_loop.Quit();
        });
    run_loop.Run();
  }
}

}  // namespace
}  // namespace syncer
