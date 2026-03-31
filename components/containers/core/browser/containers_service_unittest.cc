// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/browser/prefs_registration.h"
#include "brave/components/containers/core/browser/unknown_container.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

namespace {

mojom::ContainerPtr MakeContainer(std::string id,
                                  std::string name,
                                  mojom::Icon icon = mojom::Icon::kDefault,
                                  SkColor color = SK_ColorBLUE) {
  return mojom::Container::New(std::move(id), std::move(name), icon, color);
}

void ExpectContainer(const mojom::ContainerPtr& container,
                     const std::string& id,
                     const std::string& name,
                     const mojom::Icon& icon = mojom::Icon::kDefault,
                     const SkColor& color = SK_ColorBLUE) {
  ASSERT_TRUE(container);
  EXPECT_THAT(*container, testing::FieldsAre(id, name, icon, color));
}

}  // namespace

class ContainersServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
    RegisterProfilePrefs(prefs_.registry());

    service_ = std::make_unique<ContainersService>(&prefs_);
  }

  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<ContainersService> service_;
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

TEST_F(ContainersServiceTest, MarkContainerUsed_PersistsSnapshot) {
  auto container = MakeContainer("used-id", "Local");
  std::vector<mojom::ContainerPtr> synced;
  synced.push_back(container.Clone());
  SetContainersToPrefs(synced, prefs_);

  service_->MarkContainerUsed("used-id");

  auto used = GetUsedContainerFromPrefs(prefs_, "used-id");
  ExpectContainer(used, "used-id", "Local");
}

TEST_F(ContainersServiceTest, MarkContainerUsed_PersistsUnknownWhenNotSynced) {
  SetContainersToPrefs({}, prefs_);

  service_->MarkContainerUsed("unknown-id");

  auto used = GetUsedContainerFromPrefs(prefs_, "unknown-id");
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

  SetUsedContainerToPrefs(
      MakeContainer("used-id", "Stale", mojom::Icon::kWork, SK_ColorRED),
      prefs_);

  service_->MarkContainerUsed("used-id");

  auto used = GetUsedContainerFromPrefs(prefs_, "used-id");
  ExpectContainer(used, "used-id", "Stale", mojom::Icon::kWork, SK_ColorRED);
}

TEST_F(ContainersServiceTest,
       GetRuntimeContainerById_FallsBackToUsedWhenNotInSyncedList) {
  SetContainersToPrefs({}, prefs_);
  SetUsedContainerToPrefs(MakeContainer("cached-id", "CachedName"), prefs_);

  auto runtime = service_->GetRuntimeContainerById("cached-id");
  ExpectContainer(runtime, "cached-id", "CachedName");

  EXPECT_TRUE(service_->GetContainers().empty());
}

TEST_F(ContainersServiceTest,
       OnSyncedContainersChanged_RefreshesUsedSnapshotWhenIdStillSynced) {
  std::vector<mojom::ContainerPtr> v1;
  v1.push_back(MakeContainer("c1", "SyncedV1"));
  SetContainersToPrefs(v1, prefs_);

  SetUsedContainerToPrefs(
      MakeContainer("c1", "Stale", mojom::Icon::kWork, SK_ColorRED), prefs_);

  auto used = GetUsedContainerFromPrefs(prefs_, "c1");
  ExpectContainer(used, "c1", "Stale", mojom::Icon::kWork, SK_ColorRED);

  std::vector<mojom::ContainerPtr> v2;
  v2.push_back(MakeContainer("c1", "SyncedV2"));
  SetContainersToPrefs(v2, prefs_);

  auto used_after_sync = GetUsedContainerFromPrefs(prefs_, "c1");
  ExpectContainer(used_after_sync, "c1", "SyncedV2");
}

TEST_F(ContainersServiceTest,
       OnSyncedContainersChanged_PreservesUsedWhenIdNotInSyncedList) {
  SetUsedContainerToPrefs(MakeContainer("gone-id", "Cached"), prefs_);

  std::vector<mojom::ContainerPtr> synced;
  synced.push_back(MakeContainer("other-id", "Other"));
  SetContainersToPrefs(synced, prefs_);

  auto used = GetUsedContainerFromPrefs(prefs_, "gone-id");
  ExpectContainer(used, "gone-id", "Cached");
}

}  // namespace containers
