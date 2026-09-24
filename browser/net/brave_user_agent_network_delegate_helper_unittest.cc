/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_user_agent_network_delegate_helper.h"

#include <optional>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/net/features.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

constexpr char kSecCHUAHeader[] = "Sec-CH-UA";
constexpr char kSecCHUAFullVersionListHeader[] = "Sec-CH-UA-Full-Version-List";
constexpr char kSecCHUABrave[] =
    "\"Chromium\";v=\"140\", \"Brave\";v=\"140\", \"NotABrand\";v=\"99\"";
constexpr char kSecCHUAGoogleChrome[] =
    "\"Chromium\";v=\"140\", \"Google Chrome\";v=\"140\", "
    "\"NotABrand\";v=\"99\"";
constexpr char kSecCHUABraveFullVersionList[] =
    "\"Chromium\";v=\"140.0.0.0\", \"Not/A)Brand\";v=\"24.0.0.0\", "
    "\"Brave\";v=\"140.0.0.0\"";
constexpr char kSecCHUAGoogleChromeFullVersionList[] =
    "\"Chromium\";v=\"140.0.0.0\", \"Not/A)Brand\";v=\"24.0.0.0\", "
    "\"Google Chrome\";v=\"140.0.0.0\"";
constexpr char kSecCHUAMock[] = "Sec-CH-Mock";

namespace brave {

struct UserAgentTestResult {
  std::optional<std::string> header_value;
  std::optional<std::string> full_version_list_header_value;
};

namespace {

// Pointer strategy types for parameterized testing
struct SharedPtrStrategy {
  template <typename T>
  using Ptr = std::shared_ptr<T>;
};

struct WeakPtrStrategy {
  template <typename T>
  using Ptr = base::WeakPtr<T>;
};

}  // namespace

template <typename PtrStrategy>
class BraveUserAgentNetworkDelegateHelperTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto* exceptions =
        brave_user_agent::BraveUserAgentExceptions::GetInstance();
    exceptions->AddToExceptedDomainsForTesting("excepted.com");
    exceptions->SetIsReadyForTesting();

    // Enable feature flag if using WeakPtrStrategy, disable if
    // SharedPtrStrategy
    bool enable_flag =
        std::is_same_v<typename PtrStrategy::template Ptr<BraveRequestInfo>,
                       base::WeakPtr<BraveRequestInfo>>;
    scoped_feature_list_.InitWithFeatureState(
        features::kBraveRequestInfoUniquePtr, enable_flag);
  }

  UserAgentTestResult RunUserAgentTest(bool feature_enabled,
                                       const std::string& tab_origin,
                                       const std::string& header_name_1,
                                       const std::string& header_value_1,
                                       const std::string& header_name_2,
                                       const std::string& header_value_2) {
    base::test::ScopedFeatureList feature_list;
    if (feature_enabled) {
      feature_list.InitAndEnableFeature(
          brave_user_agent::features::kUseBraveUserAgent);
    } else {
      feature_list.InitAndDisableFeature(
          brave_user_agent::features::kUseBraveUserAgent);
    }
    net::HttpRequestHeaders headers;
    headers.SetHeader(header_name_1, header_value_1);
    headers.SetHeader(header_name_2, header_value_2);

    typename PtrStrategy::template Ptr<BraveRequestInfo> ctx;
    if constexpr (std::is_same_v<
                      typename PtrStrategy::template Ptr<BraveRequestInfo>,
                      std::shared_ptr<BraveRequestInfo>>) {
      ctx = std::make_shared<BraveRequestInfo>();
    } else {
      owned_request_ = std::make_unique<BraveRequestInfo>();
      ctx = owned_request_->AsWeakPtr();
    }
    ctx->set_tab_origin(GURL(tab_origin));
    int result = OnBeforeStartTransaction_UserAgentWork(&headers, {}, ctx);
    EXPECT_EQ(result, net::OK);
    return {headers.GetHeader(kSecCHUAHeader),
            headers.GetHeader(kSecCHUAFullVersionListHeader)};
  }

  // For WeakPtr tests, store ownership of the request object
  std::unique_ptr<BraveRequestInfo> owned_request_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

using PtrStrategies = testing::Types<SharedPtrStrategy, WeakPtrStrategy>;
TYPED_TEST_SUITE(BraveUserAgentNetworkDelegateHelperTest, PtrStrategies);

TYPED_TEST(BraveUserAgentNetworkDelegateHelperTest,
           ReplacesBraveWithGoogleChromeIfExcepted) {
  auto res = this->RunUserAgentTest(
      /*feature_enabled=*/true, "https://excepted.com", kSecCHUAHeader,
      kSecCHUABrave, kSecCHUAFullVersionListHeader,
      kSecCHUABraveFullVersionList);
  ASSERT_TRUE(res.header_value.has_value());
  ASSERT_TRUE(res.full_version_list_header_value.has_value());
  EXPECT_EQ(res.header_value.value(), kSecCHUAGoogleChrome);
  EXPECT_EQ(res.full_version_list_header_value.value(),
            kSecCHUAGoogleChromeFullVersionList);
}

TYPED_TEST(BraveUserAgentNetworkDelegateHelperTest,
           DoesNotReplaceBraveWithGoogleChromeIfNotExcepted) {
  auto res = this->RunUserAgentTest(
      /*feature_enabled=*/true, "https://not-excepted.com", kSecCHUAHeader,
      kSecCHUABrave, kSecCHUAFullVersionListHeader,
      kSecCHUABraveFullVersionList);
  ASSERT_TRUE(res.header_value.has_value());
  ASSERT_TRUE(res.full_version_list_header_value.has_value());
  EXPECT_EQ(res.header_value.value(), kSecCHUABrave);
  EXPECT_EQ(res.full_version_list_header_value.value(),
            kSecCHUABraveFullVersionList);
}

TYPED_TEST(BraveUserAgentNetworkDelegateHelperTest,
           DoesNotReplaceBraveWithGoogleChromeIfHeaderNotSet) {
  auto res = this->RunUserAgentTest(
      /*feature_enabled=*/true, "https://excepted.com", kSecCHUAMock,
      kSecCHUABrave, kSecCHUAMock, kSecCHUABraveFullVersionList);
  EXPECT_FALSE(res.header_value.has_value());
  EXPECT_FALSE(res.full_version_list_header_value.has_value());
}

TYPED_TEST(BraveUserAgentNetworkDelegateHelperTest,
           DoesNotReplaceBraveWithGoogleChromeIfHeaderSetToEmpty) {
  auto res = this->RunUserAgentTest(
      /*feature_enabled=*/true, "https://excepted.com", kSecCHUAHeader, "",
      kSecCHUAFullVersionListHeader, "");
  ASSERT_TRUE(res.header_value.has_value());
  ASSERT_TRUE(res.full_version_list_header_value.has_value());
  EXPECT_EQ(res.header_value.value(), "");
  EXPECT_EQ(res.full_version_list_header_value.value(), "");
  EXPECT_THAT(*res.header_value, testing::Not(testing::HasSubstr("\"Brave\"")));
  EXPECT_THAT(*res.full_version_list_header_value,
              testing::Not(testing::HasSubstr("\"Brave\"")));
  EXPECT_THAT(*res.header_value,
              testing::Not(testing::HasSubstr("\"Google Chrome\"")));
  EXPECT_THAT(res.full_version_list_header_value.value(),
              testing::Not(testing::HasSubstr("\"Google Chrome\"")));
}

TYPED_TEST(BraveUserAgentNetworkDelegateHelperTest,
           DoesNotReplaceBraveWithGoogleChromeIfFeatureIsDisabled) {
  auto res = this->RunUserAgentTest(
      /*feature_enabled=*/false, "https://excepted.com", kSecCHUAHeader,
      kSecCHUABrave, kSecCHUAFullVersionListHeader,
      kSecCHUABraveFullVersionList);
  ASSERT_TRUE(res.header_value.has_value());
  ASSERT_TRUE(res.full_version_list_header_value.has_value());
  EXPECT_EQ(res.header_value.value(), kSecCHUABrave);
  EXPECT_EQ(res.full_version_list_header_value.value(),
            kSecCHUABraveFullVersionList);
}

}  // namespace brave
