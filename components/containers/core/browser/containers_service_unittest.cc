// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/browser/containers_service_observer.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/browser/prefs_registration.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/browser/unknown_container.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/color/color_provider_manager.h"

namespace containers {

namespace {

class MockContainersServiceObserver : public ContainersServiceObserver {
 public:
  MOCK_METHOD(void, OnContainersListChanged, (), (override));
};

}  // namespace

class ContainersServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
    RegisterProfilePrefs(prefs_.registry());

    auto delegate =
        std::make_unique<testing::NiceMock<MockContainersServiceDelegate>>();
    delegate_ = delegate.get();
    service_ =
        std::make_unique<ContainersService>(&prefs_, std::move(delegate));
  }

  void TearDown() override {
    delegate_ = nullptr;
    service_->Shutdown();
    service_.reset();
    ui::ColorProviderManager::ResetForTesting();
  }

  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<ContainersService> service_;
  raw_ptr<MockContainersServiceDelegate> delegate_ = nullptr;
};

TEST_F(ContainersServiceTest, GetRuntimeContainerById) {
  auto container = MakeContainer("container-id", "Personal");
  std::vector<mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(synced_containers), prefs_);

  auto runtime_container = service_->GetRuntimeContainerById("container-id");
  ExpectContainer(runtime_container, "container-id", "Personal");

  EXPECT_FALSE(service_->GetRuntimeContainerById("non-existent-container-id"));
}

TEST_F(ContainersServiceTest, GetContainers) {
  auto container = MakeContainer("container-id", "Work");
  std::vector<mojom::ContainerPtr> containers;
  containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(containers), prefs_);

  auto containers_list = service_->GetContainers();
  ExpectContainer(containers_list[0], "container-id", "Work");
}

TEST_F(ContainersServiceTest, CreateAndPersistTemporaryContainer) {
  auto container = service_->CreateAndPersistTemporaryContainer();
  ASSERT_TRUE(container);
  EXPECT_TRUE(IsTemporaryContainerId(container->id));
  EXPECT_FALSE(container->name.empty());
  EXPECT_GE(container->icon, mojom::Icon::kMinValue);
  EXPECT_LE(container->icon, mojom::Icon::kMaxValue);
  EXPECT_NE(SK_ColorTRANSPARENT, container->background_color);

  auto persisted = GetLocallyUsedContainerFromPrefs(prefs_, container->id);
  ASSERT_TRUE(persisted);
  ExpectContainer(persisted, container->id, container->name, container->icon,
                  container->background_color);
}

TEST_F(ContainersServiceTest,
       Observer_OnContainersListChanged_WhenContainersListPrefChanges) {
  testing::NiceMock<MockContainersServiceObserver> observer;
  EXPECT_CALL(observer, OnContainersListChanged()).Times(1);

  service_->AddObserver(&observer);

  std::vector<mojom::ContainerPtr> containers;
  containers.push_back(MakeContainer("container-id", "Work"));
  SetContainersToPrefs(containers, prefs_);
  testing::Mock::VerifyAndClearExpectations(&observer);

  EXPECT_CALL(observer, OnContainersListChanged()).Times(1);
  SetContainersToPrefs({}, prefs_);
  testing::Mock::VerifyAndClearExpectations(&observer);

  service_->RemoveObserver(&observer);
}

TEST_F(ContainersServiceTest, MarkContainerUsed_PersistsSnapshot) {
  auto container = MakeContainer("used-id", "Local");
  std::vector<mojom::ContainerPtr> synced;
  synced.push_back(container.Clone());
  SetContainersToPrefs(synced, prefs_);

  service_->MarkContainerUsed("used-id");

  auto used = GetLocallyUsedContainerFromPrefs(prefs_, "used-id");
  ExpectContainer(used, "used-id", "Local");
}

TEST_F(ContainersServiceTest, MarkContainerUsed_PersistsUnknownWhenNotSynced) {
  SetContainersToPrefs({}, prefs_);

  service_->MarkContainerUsed("unknown-id");

  auto used = GetLocallyUsedContainerFromPrefs(prefs_, "unknown-id");
  ExpectContainer(used, "unknown-id", "unknown-", mojom::Icon::kDefault,
                  kUnknownContainerBackgroundColor);
}

TEST_F(ContainersServiceTest,
       MarkContainerUsed_DoesNotUpsertWhenAlreadyInUsedPrefs) {
  auto container = MakeContainer("used-id", "Local");
  std::vector<mojom::ContainerPtr> synced;
  synced.push_back(container.Clone());
  SetContainersToPrefs(synced, prefs_);

  service_->MarkContainerUsed("used-id");

  SetLocallyUsedContainerToPrefs(
      MakeContainer("used-id", "Stale", mojom::Icon::kWork, SK_ColorRED),
      prefs_);

  service_->MarkContainerUsed("used-id");

  auto used = GetLocallyUsedContainerFromPrefs(prefs_, "used-id");
  ExpectContainer(used, "used-id", "Stale", mojom::Icon::kWork, SK_ColorRED);
}

TEST_F(ContainersServiceTest,
       GetRuntimeContainerById_FallsBackToUsedWhenNotInSyncedList) {
  SetContainersToPrefs({}, prefs_);
  SetLocallyUsedContainerToPrefs(MakeContainer("cached-id", "CachedName"),
                                 prefs_);

  auto runtime = service_->GetRuntimeContainerById("cached-id");
  ExpectContainer(runtime, "cached-id", "CachedName");

  EXPECT_TRUE(service_->GetContainers().empty());
}

TEST_F(ContainersServiceTest,
       OnSyncedContainersChanged_RefreshesUsedSnapshotWhenIdStillSynced) {
  std::vector<mojom::ContainerPtr> v1;
  v1.push_back(MakeContainer("c1", "SyncedV1"));
  SetContainersToPrefs(v1, prefs_);

  SetLocallyUsedContainerToPrefs(
      MakeContainer("c1", "Stale", mojom::Icon::kWork, SK_ColorRED), prefs_);

  auto used = GetLocallyUsedContainerFromPrefs(prefs_, "c1");
  ExpectContainer(used, "c1", "Stale", mojom::Icon::kWork, SK_ColorRED);

  std::vector<mojom::ContainerPtr> v2;
  v2.push_back(MakeContainer("c1", "SyncedV2"));
  SetContainersToPrefs(v2, prefs_);

  auto used_after_sync = GetLocallyUsedContainerFromPrefs(prefs_, "c1");
  ExpectContainer(used_after_sync, "c1", "SyncedV2");
}

TEST_F(ContainersServiceTest,
       OnSyncedContainersChanged_PreservesUsedWhenIdNotInSyncedList) {
  SetLocallyUsedContainerToPrefs(MakeContainer("gone-id", "Cached"), prefs_);

  std::vector<mojom::ContainerPtr> synced;
  synced.push_back(MakeContainer("other-id", "Other"));
  SetContainersToPrefs(synced, prefs_);

  auto used = GetLocallyUsedContainerFromPrefs(prefs_, "gone-id");
  ExpectContainer(used, "gone-id", "Cached");
}

TEST_F(ContainersServiceTest, CleanupRunsWhenReferencesDisappear) {
  auto container = MakeContainer("container-id", "Shopping",
                                 mojom::Icon::kShopping, SK_ColorRED);
  std::vector<mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(synced_containers), prefs_);
  service_->MarkContainerUsed("container-id");

  delegate_->SetReferencedContainersIds({});
  SetContainersToPrefs({}, prefs_);

  service_->ScheduleOrphanedContainersCleanupForTesting();

  ASSERT_EQ(delegate_->delete_requests().size(), 1u);
  EXPECT_EQ(delegate_->delete_requests()[0], "container-id");
  EXPECT_FALSE(GetLocallyUsedContainerFromPrefs(prefs_, "container-id"));
}

TEST_F(ContainersServiceTest, CleanupBlockedWhileContainerIsReferenced) {
  auto container =
      MakeContainer("container-id", "Work", mojom::Icon::kWork, SK_ColorGREEN);
  std::vector<mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(synced_containers), prefs_);
  service_->MarkContainerUsed("container-id");

  delegate_->SetReferencedContainersIds({"container-id"});
  SetContainersToPrefs({}, prefs_);

  service_->ScheduleOrphanedContainersCleanupForTesting();

  EXPECT_TRUE(delegate_->delete_requests().empty());
  EXPECT_TRUE(GetLocallyUsedContainerFromPrefs(prefs_, "container-id"));
}

TEST_F(ContainersServiceTest, CleanupFailureKeepsUsedSnapshot) {
  auto container = MakeContainer("container-id", "Shopping",
                                 mojom::Icon::kShopping, SK_ColorRED);
  std::vector<mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(synced_containers), prefs_);
  service_->MarkContainerUsed("container-id");

  delegate_->SetReferencedContainersIds({});
  SetContainersToPrefs({}, prefs_);

  delegate_->set_delete_result(false);
  service_->ScheduleOrphanedContainersCleanupForTesting();

  ASSERT_EQ(delegate_->delete_requests().size(), 1u);
  EXPECT_TRUE(GetLocallyUsedContainerFromPrefs(prefs_, "container-id"));
}

}  // namespace containers
