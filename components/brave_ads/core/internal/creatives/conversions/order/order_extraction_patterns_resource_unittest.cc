/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/test/run_until.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_load_state_types.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/resource_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsOrderExtractionPatternsResourceTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<OrderExtractionPatternsResource>();
  }

  std::unique_ptr<OrderExtractionPatternsResource> resource_;
};

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       ReturnDefaultPatternsIfResourceNotLoaded) {
  // Act & Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
  const OrderExtractionPatternsInfo* const patterns = resource_->GetPatterns();
  ASSERT_TRUE(patterns);
  EXPECT_FALSE(patterns->id.regexes.empty());
  EXPECT_FALSE(patterns->value.currency_regexes.empty());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest, LoadResource) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));
  const OrderExtractionPatternsInfo* const patterns = resource_->GetPatterns();
  ASSERT_TRUE(patterns);
  EXPECT_FALSE(patterns->id.regexes.empty());
  EXPECT_FALSE(patterns->value.currency_regexes.empty());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoNotLoadMalformedResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kOrderExtractionPatternsResourceId));

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad;
  }));
  EXPECT_TRUE(resource_->GetPatterns());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoNotLoadResourceWithUnsupportedVersion) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/
      test::kUnsupportedVersionOrderExtractionPatternsResourceId,
      /*to_path=*/kOrderExtractionPatternsResourceId));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
  EXPECT_TRUE(resource_->GetPatterns());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_,
          LoadResourceComponent(kOrderExtractionPatternsResourceId,
                                /*version=*/::testing::_,
                                /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadResourceComponentCallback callback) {
        const base::FilePath path =
            test::ResourceComponentsDataPath().AppendASCII(
                test::kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file), /*exists=*/true);
      });

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad;
  }));
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoNotFlagFailureForUnregisteredResource) {
  // Arrange
  ON_CALL(ads_client_mock_,
          LoadResourceComponent(kOrderExtractionPatternsResourceId,
                                /*version=*/::testing::_,
                                /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadResourceComponentCallback callback) {
        std::move(callback).Run(/*file=*/{}, /*exists=*/false);
      });

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoNotLoadResourceWithInvalidLanguageComponentId) {
  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion,
      test::kInvalidLanguageComponentId);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsOrderExtractionPatternsResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithExistingManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       ResetResourceForOnNotifyDidUnregisterResourceComponent) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kLanguageComponentId);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
  EXPECT_TRUE(resource_->GetPatterns());
}

TEST_F(
    BraveAdsOrderExtractionPatternsResourceTest,
    DoNotResetResourceForOnNotifyDidUnregisterResourceComponentWithInvalidLanguageComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kInvalidLanguageComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

}  // namespace brave_ads
