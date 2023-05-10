/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/resources/resources_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using testing::_;
using testing::Invoke;

namespace {
constexpr char kResourceId[] = "mkdhnfmjhklfnamlheoliekgeohamoig";
}  // namespace

class BraveAdsAntiTargetingResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<AntiTargetingResource>();
  }

  bool LoadResource() {
    resource_->Load();
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<AntiTargetingResource> resource_;
};

TEST_F(BraveAdsAntiTargetingResourceTest, LoadResource) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(LoadResource());
}

TEST_F(BraveAdsAntiTargetingResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidResourceId, kResourceId);

  // Act

  // Assert
  EXPECT_FALSE(LoadResource());
}

TEST_F(BraveAdsAntiTargetingResourceTest, DoNotLoadMissingResource) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, LoadFileResource(kResourceId, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  // Act

  // Assert
  EXPECT_FALSE(LoadResource());
}

TEST_F(BraveAdsAntiTargetingResourceTest, IsNotInitialized) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

}  // namespace brave_ads
