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
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

namespace {

mojom::ContainerPtr MakeContainer(std::string id,
                                  std::string name,
                                  mojom::Icon icon,
                                  SkColor color) {
  return mojom::Container::New(std::move(id), std::move(name), icon, color);
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
  auto container = MakeContainer("container-id", "Personal",
                                 mojom::Icon::kPersonal, SK_ColorBLUE);
  std::vector<mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(synced_containers), prefs_);

  auto runtime_container = service_->GetRuntimeContainerById("container-id");
  ASSERT_TRUE(runtime_container);
  EXPECT_EQ(runtime_container->name, "Personal");
  EXPECT_EQ(runtime_container->icon, mojom::Icon::kPersonal);
  EXPECT_EQ(runtime_container->background_color, SK_ColorBLUE);

  EXPECT_FALSE(service_->GetRuntimeContainerById("non-existent-container-id"));
}

TEST_F(ContainersServiceTest, GetContainers) {
  auto container =
      MakeContainer("container-id", "Work", mojom::Icon::kWork, SK_ColorGREEN);
  std::vector<mojom::ContainerPtr> containers;
  containers.push_back(container.Clone());
  SetContainersToPrefs(std::move(containers), prefs_);

  auto containers_list = service_->GetContainers();
  ASSERT_EQ(containers_list.size(), 1u);
  EXPECT_EQ(containers_list[0]->name, "Work");
  EXPECT_EQ(containers_list[0]->icon, mojom::Icon::kWork);
  EXPECT_EQ(containers_list[0]->background_color, SK_ColorGREEN);
}

}  // namespace containers
