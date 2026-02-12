/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"

#include <memory>
#include <string_view>

#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// The list of exceptions contains TLDs+1
constexpr char kExceptionList[] = "a.com";

}  // namespace

class HttpsUpgradeExceptionsServiceBaseTest : public testing::Test {
 public:
  void SetUp() override {
    local_data_files_service_ =
        brave_component_updater::LocalDataFilesServiceFactory(nullptr);
    https_upgrade_exceptions_service_ = std::make_unique<
        https_upgrade_exceptions::HttpsUpgradeExceptionsService>(
        local_data_files_service_.get());
    ASSERT_TRUE(https_upgrade_exceptions_service_);

    testing::Test::SetUp();
  }

  https_upgrade_exceptions::HttpsUpgradeExceptionsService*
  https_upgrade_exceptions_service() {
    return https_upgrade_exceptions_service_.get();
  }

 private:
  std::unique_ptr<brave_component_updater::LocalDataFilesService>
      local_data_files_service_;
  std::unique_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>
      https_upgrade_exceptions_service_;
};

class HttpsUpgradeExceptionsServiceFeatureDisabledTest
    : public HttpsUpgradeExceptionsServiceBaseTest {
 public:
  HttpsUpgradeExceptionsServiceFeatureDisabledTest() {
    feature_list_.InitAndDisableFeature(net::features::kBraveHttpsByDefault);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(HttpsUpgradeExceptionsServiceFeatureDisabledTest,
       CanUpgradeToHTTPS_DisabledFeature) {
  const auto a_url = GURL("http://a.com");
  EXPECT_FALSE(https_upgrade_exceptions_service()->CanUpgradeToHTTPS(a_url));

  https_upgrade_exceptions_service()->OnDATFileDataReady(kExceptionList);
  EXPECT_FALSE(https_upgrade_exceptions_service()->CanUpgradeToHTTPS(a_url));
}

class HttpsUpgradeExceptionsServiceFeatureEnabledTest
    : public HttpsUpgradeExceptionsServiceBaseTest {
 public:
  HttpsUpgradeExceptionsServiceFeatureEnabledTest() {
    feature_list_.InitAndEnableFeature(net::features::kBraveHttpsByDefault);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(HttpsUpgradeExceptionsServiceFeatureEnabledTest,
       CanUpgradeToHTTPS_NotReady) {
  const auto a_url = GURL("http://a.com");
  EXPECT_FALSE(https_upgrade_exceptions_service()->CanUpgradeToHTTPS(a_url));
}

TEST_F(HttpsUpgradeExceptionsServiceFeatureEnabledTest, CanUpgradeToHTTPS) {
  struct TestCases {
    std::string name;
    GURL url;
    bool can_upgrade;
  } kTestCases[] = {
      {"Do not upgrade as exception", GURL("http://a.com"), false},
      {"Do not upgrade subdomain as exception", GURL("http://sub.a.com"),
       false},
      {"Do not upgrade as wrong schema", GURL("chrome://a.com"), false},
      {"Simple Upgrade to HTTPS", GURL("http://b.com"), true}};

  https_upgrade_exceptions_service()->OnDATFileDataReady(kExceptionList);

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message() << test_case.name);
    EXPECT_EQ(
        https_upgrade_exceptions_service()->CanUpgradeToHTTPS(test_case.url),
        test_case.can_upgrade);
  }
}
