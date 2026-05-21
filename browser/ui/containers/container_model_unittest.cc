// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/container_model.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/browser/prefs_registration.h"
#include "brave/components/containers/core/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainerModelUnitTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
    RegisterProfilePrefs(prefs_.registry());
    service_ = std::make_unique<ContainersService>(
        &prefs_,
        std::make_unique<testing::NiceMock<MockContainersServiceDelegate>>());
  }

  void TearDown() override {
    service_->Shutdown();
    service_.reset();
  }

  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<ContainersService> service_;
};

TEST_F(ContainerModelUnitTest, IncludesCurrentRuntimeContainers) {
  std::vector<mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(mojom::Container::New(
      "synced-id", "Synced", mojom::Icon::kDefault, SK_ColorBLUE));
  SetContainersToPrefs(std::move(synced_containers), prefs_);
  SetLocallyUsedContainerToPrefs(
      mojom::Container::New("cached-id", "Cached", mojom::Icon::kDefault,
                            SK_ColorBLUE),
      prefs_);
  SetLocallyUsedContainerToPrefs(
      mojom::Container::New("unused-id", "Unused", mojom::Icon::kDefault,
                            SK_ColorBLUE),
      prefs_);

  const std::vector<ContainerModel> models =
      GetContainerModels(*service_, base::flat_set<std::string>{"cached-id"},
                         /*scale_factor=*/1.0f);

  ASSERT_EQ(models.size(), 2u);
  EXPECT_EQ(models[0].id(), "synced-id");
  EXPECT_EQ(models[1].id(), "cached-id");
}

}  // namespace containers
