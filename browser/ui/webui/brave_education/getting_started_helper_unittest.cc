/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/getting_started_helper.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/brave_education/common/education_content_urls.h"
#include "brave/components/brave_education/common/features.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_education {

class GettingStartedHelperTest : public testing::Test {
 protected:
  void AddSuccessResponse() {
    auto url =
        GetEducationContentServerURL(EducationContentType::kGettingStarted);
    test_url_loader_factory_.AddResponse(url.spec(), "success");
  }

  void AddErrorResponse() {
    auto url =
        GetEducationContentServerURL(EducationContentType::kGettingStarted);
    test_url_loader_factory_.AddResponse(url.spec(), "error",
                                         net::HTTP_NOT_FOUND);
  }

  std::unique_ptr<TestingProfile> BuildProfile() {
    TestingProfile::Builder builder;
    builder.SetSharedURLLoaderFactory(
        test_url_loader_factory_.GetSafeWeakWrapper());
    return builder.Build();
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

TEST_F(GettingStartedHelperTest, WithDefaultContentType) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kShowGettingStartedPage);

  AddSuccessResponse();

  auto profile = BuildProfile();

  base::test::TestFuture<std::optional<GURL>> future;
  GettingStartedHelper helper(profile.get());
  helper.GetEducationURL(future.GetCallback());
  ASSERT_TRUE(future.Wait());

  auto result = future.Get<0>();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result.value(), GetEducationContentBrowserURL(
                                EducationContentType::kGettingStarted));
}

TEST_F(GettingStartedHelperTest, FeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kShowGettingStartedPage);

  AddSuccessResponse();

  auto profile = BuildProfile();

  base::test::TestFuture<std::optional<GURL>> future;
  GettingStartedHelper helper(profile.get());
  helper.GetEducationURL(future.GetCallback());
  ASSERT_TRUE(future.Wait());

  auto result = future.Get<0>();
  ASSERT_FALSE(result.has_value());
}

TEST_F(GettingStartedHelperTest, BadResponse) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kShowGettingStartedPage);

  AddErrorResponse();

  auto profile = BuildProfile();

  base::test::TestFuture<std::optional<GURL>> future;
  GettingStartedHelper helper(profile.get());
  helper.GetEducationURL(future.GetCallback());
  ASSERT_TRUE(future.Wait());

  auto result = future.Get<0>();
  ASSERT_FALSE(result.has_value());
}

}  // namespace brave_education
