// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_sync_bridge.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/data_type_store.h"
#include "components/sync/model/entity_change.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/test/data_type_store_test_util.h"
#include "components/sync/test/mock_data_type_local_change_processor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

using ::testing::_;
using ::testing::Return;

sync_pb::BraveNewsSpecifics MakeBoolSpecifics(const std::string& name,
                                              bool value) {
  sync_pb::BraveNewsSpecifics specifics;
  specifics.set_name(name);
  specifics.set_bool_value(value);
  return specifics;
}

sync_pb::BraveNewsSpecifics MakeDictSpecifics(const std::string& name,
                                              const std::string& json) {
  sync_pb::BraveNewsSpecifics specifics;
  specifics.set_name(name);
  specifics.set_dict_value(json);
  return specifics;
}

syncer::EntityData EntityFromSpecifics(
    const sync_pb::BraveNewsSpecifics& specifics) {
  syncer::EntityData entity;
  *entity.specifics.mutable_brave_news() = specifics;
  return entity;
}

}  // namespace

class BraveNewsSyncBridgeTest : public testing::Test {
 public:
  BraveNewsSyncBridgeTest()
      : store_(syncer::DataTypeStoreTestUtil::CreateInMemoryStoreForTest()) {
    prefs_.registry()->RegisterDictionaryPref(prefs::kBraveNewsSources);
    prefs_.registry()->RegisterDictionaryPref(prefs::kBraveNewsChannels);
    prefs_.registry()->RegisterDictionaryPref(prefs::kBraveNewsDirectFeeds);
    prefs_.registry()->RegisterBooleanPref(prefs::kShouldShowToolbarButton,
                                           true);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kBraveNewsOpenArticlesInNewTab, true);
    RecreateBridge();
  }

  void RecreateBridge() {
    bridge_ = std::make_unique<BraveNewsSyncBridge>(
        &prefs_, mock_processor_.CreateForwardingProcessor(),
        syncer::DataTypeStoreTestUtil::FactoryForForwardingStore(store_.get()));
    task_environment_.RunUntilIdle();
  }

  // Drives MergeFullSyncData and marks the bridge as tracking metadata
  // afterwards, mirroring a successful initial sync.
  void StartSyncing(
      const std::vector<sync_pb::BraveNewsSpecifics>& remote_specifics) {
    syncer::EntityChangeList change_list;
    for (const auto& specifics : remote_specifics) {
      change_list.push_back(syncer::EntityChange::CreateAdd(
          specifics.name(), EntityFromSpecifics(specifics)));
    }
    const bool success = !bridge_->MergeFullSyncData(
        bridge_->CreateMetadataChangeList(), std::move(change_list));
    ON_CALL(mock_processor_, IsTrackingMetadata)
        .WillByDefault(Return(success));
    task_environment_.RunUntilIdle();
  }

  void ApplyChanges(syncer::EntityChangeList changes) {
    bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                         std::move(changes));
    task_environment_.RunUntilIdle();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<syncer::DataTypeStore> store_;
  testing::NiceMock<syncer::MockDataTypeLocalChangeProcessor> mock_processor_;
  std::unique_ptr<BraveNewsSyncBridge> bridge_;
};

TEST_F(BraveNewsSyncBridgeTest, ModelReadyToSyncOnCreate) {
  EXPECT_CALL(mock_processor_, ModelReadyToSync);
  RecreateBridge();
}

TEST_F(BraveNewsSyncBridgeTest, GetStorageKeyAndClientTag) {
  syncer::EntityData entity =
      EntityFromSpecifics(MakeBoolSpecifics(prefs::kShouldShowToolbarButton, true));
  EXPECT_EQ(bridge_->GetStorageKey(entity), prefs::kShouldShowToolbarButton);
  EXPECT_EQ(bridge_->GetClientTag(entity), prefs::kShouldShowToolbarButton);
}

TEST_F(BraveNewsSyncBridgeTest, IsEntityDataValid) {
  // A known synced pref name is valid.
  EXPECT_TRUE(bridge_->IsEntityDataValid(
      EntityFromSpecifics(MakeBoolSpecifics(prefs::kShouldShowToolbarButton, true))));
  // An unknown pref name is rejected.
  EXPECT_FALSE(bridge_->IsEntityDataValid(
      EntityFromSpecifics(MakeBoolSpecifics("some.other.pref", true))));
  // Missing name is rejected.
  EXPECT_FALSE(bridge_->IsEntityDataValid(
      EntityFromSpecifics(sync_pb::BraveNewsSpecifics())));
}

TEST_F(BraveNewsSyncBridgeTest, RemoteBoolChangeWritesPrefWithoutEcho) {
  StartSyncing(/*remote_specifics=*/{});
  ASSERT_TRUE(prefs_.GetBoolean(prefs::kShouldShowToolbarButton));

  // Applying a remote change must update the pref but must NOT echo back out as
  // a local Put.
  EXPECT_CALL(mock_processor_, Put).Times(0);
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      prefs::kShouldShowToolbarButton,
      EntityFromSpecifics(
          MakeBoolSpecifics(prefs::kShouldShowToolbarButton, false))));
  ApplyChanges(std::move(changes));

  EXPECT_FALSE(prefs_.GetBoolean(prefs::kShouldShowToolbarButton));
}

TEST_F(BraveNewsSyncBridgeTest, RemoteDictChangeRoundTrips) {
  StartSyncing(/*remote_specifics=*/{});

  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      prefs::kBraveNewsSources,
      EntityFromSpecifics(MakeDictSpecifics(prefs::kBraveNewsSources,
                                            R"({"publisher-1":true})"))));
  ApplyChanges(std::move(changes));

  const base::DictValue& sources = prefs_.GetDict(prefs::kBraveNewsSources);
  EXPECT_THAT(sources.FindBool("publisher-1"), testing::Optional(true));
}

TEST_F(BraveNewsSyncBridgeTest, LocalPrefChangeTriggersPut) {
  StartSyncing(/*remote_specifics=*/{});

  EXPECT_CALL(mock_processor_,
              Put(std::string(prefs::kBraveNewsOpenArticlesInNewTab), _, _));
  prefs_.SetBoolean(prefs::kBraveNewsOpenArticlesInNewTab, false);
  task_environment_.RunUntilIdle();
}

TEST_F(BraveNewsSyncBridgeTest, MergeUploadsLocalOnlyPrefs) {
  // A device that configured Brave News before enabling sync.
  prefs_.SetBoolean(prefs::kShouldShowToolbarButton, false);

  // No remote entity for this pref -> it must be uploaded on merge.
  EXPECT_CALL(mock_processor_,
              Put(std::string(prefs::kShouldShowToolbarButton), _, _));
  StartSyncing(/*remote_specifics=*/{});
}

TEST_F(BraveNewsSyncBridgeTest, RemoteDeleteResetsPrefToDefault) {
  StartSyncing(/*remote_specifics=*/{});
  prefs_.SetBoolean(prefs::kShouldShowToolbarButton, false);
  task_environment_.RunUntilIdle();

  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateDelete(
      prefs::kShouldShowToolbarButton, syncer::EntityData()));
  ApplyChanges(std::move(changes));

  // Cleared back to the registered default (true).
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kShouldShowToolbarButton));
}

}  // namespace brave_news
