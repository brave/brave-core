/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/search_ads_header_network_delegate_helper.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/net/features.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-data-view.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_rewards/core/features.h"
#endif  // BUILDFLAG(IS_ANDROID)

using testing::Return;

namespace brave {

namespace {

constexpr std::string_view kBraveSearchRequestUrl =
    "https://search.brave.com/search?q=qwerty";
constexpr std::string_view kBraveSearchImageRequestUrl =
    "https://search.brave.com/img.png";
constexpr std::string_view kNonBraveSearchRequestUrl =
    "https://brave.com/search?q=qwerty";
constexpr std::string_view kBraveSearchTabUrl = "https://search.brave.com";
constexpr std::string_view kNonBraveSearchTabUrl = "https://brave.com";

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
class SearchAdsHeaderDelegateHelperTest : public testing::Test {
 protected:
  void SetUp() override {
    // Enable feature flag if using WeakPtrStrategy, disable if
    // SharedPtrStrategy
    bool enable_flag =
        std::is_same_v<typename PtrStrategy::template Ptr<BraveRequestInfo>,
                       base::WeakPtr<BraveRequestInfo>>;
    scoped_feature_list_.InitWithFeatureStates({
#if BUILDFLAG(IS_ANDROID)
        {brave_rewards::features::kBraveRewards, true},
#endif  // BUILDFLAG(IS_ANDROID)
        {features::kBraveRequestInfoUniquePtr, enable_flag}});
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
  }

  void EnableBraveRewards() {
    // Disabled by default.
    profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  }

  void ConnectExternalBraveRewardsWallet() {
    // Disconnected by default.
    profile_->GetPrefs()->SetString(brave_rewards::prefs::kExternalWalletType,
                                    "connected");
  }

  void OptOutOfSearchResultAds() {
    // Opted-in by default.
    profile_->GetPrefs()->SetBoolean(
        brave_ads::prefs::kOptedInToSearchResultAds, false);
  }

  typename PtrStrategy::template Ptr<BraveRequestInfo> MakeRequest(
      std::string_view url,
      TestingProfile* profile) {
    if constexpr (std::is_same_v<
                      typename PtrStrategy::template Ptr<BraveRequestInfo>,
                      std::shared_ptr<BraveRequestInfo>>) {
      auto request = std::make_shared<BraveRequestInfo>(GURL(url));
      request->set_request_url(GURL(kBraveSearchTabUrl));
      request->set_tab_origin(GURL(kBraveSearchTabUrl));
      request->set_initiator_url(GURL(kBraveSearchTabUrl));
      request->set_resource_type(blink::mojom::ResourceType::kMainFrame);
      request->set_browser_context(profile);
      return request;
    } else {
      // For WeakPtr strategy, store ownership in unique_ptr member
      owned_request_ = std::make_unique<BraveRequestInfo>(GURL(url));
      owned_request_->set_request_url(GURL(kBraveSearchTabUrl));
      owned_request_->set_tab_origin(GURL(kBraveSearchTabUrl));
      owned_request_->set_initiator_url(GURL(kBraveSearchTabUrl));
      owned_request_->set_resource_type(blink::mojom::ResourceType::kMainFrame);
      owned_request_->set_browser_context(profile);
      return owned_request_->AsWeakPtr();
    }
  }

  void VerifyHeaderExistsExpectation(
      typename PtrStrategy::template Ptr<BraveRequestInfo> request) {
    net::HttpRequestHeaders request_headers;
    const int result_code = OnBeforeStartTransaction_SearchAdsHeader(
        &request_headers, ResponseCallback(), request);
    EXPECT_EQ(result_code, net::OK);
    EXPECT_TRUE(request_headers.HasHeader(kSearchAdsHeader));
    EXPECT_EQ(request_headers.GetHeader(kSearchAdsHeader),
              kSearchAdsDisabledValue);
  }

  void VerifyMissingHeaderExpectation(
      typename PtrStrategy::template Ptr<BraveRequestInfo> request) {
    net::HttpRequestHeaders request_headers;
    const int result_code = OnBeforeStartTransaction_SearchAdsHeader(
        &request_headers, ResponseCallback(), request);
    EXPECT_EQ(result_code, net::OK);
    EXPECT_FALSE(request_headers.HasHeader(kSearchAdsHeader));
  }

  brave_l10n::test::ScopedDefaultLocale scoped_locale_{"en_US"};
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<TestingProfile> profile_;
  // For WeakPtr tests, store ownership of the request object
  std::unique_ptr<BraveRequestInfo> owned_request_;
};

using PtrStrategies = testing::Types<SharedPtrStrategy, WeakPtrStrategy>;
TYPED_TEST_SUITE(SearchAdsHeaderDelegateHelperTest, PtrStrategies);

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldExistForMainFrameResource) {
  this->EnableBraveRewards();
  this->OptOutOfSearchResultAds();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_resource_type(blink::mojom::ResourceType::kMainFrame);
  this->VerifyHeaderExistsExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest, HeaderShouldExistForXhrResource) {
  this->EnableBraveRewards();
  this->OptOutOfSearchResultAds();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_resource_type(blink::mojom::ResourceType::kXhr);
  this->VerifyHeaderExistsExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldExistForImageResource) {
  this->EnableBraveRewards();
  this->OptOutOfSearchResultAds();

  auto request =
      this->MakeRequest(kBraveSearchImageRequestUrl, this->profile_.get());
  request->set_resource_type(blink::mojom::ResourceType::kImage);
  this->VerifyHeaderExistsExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForDisallowedTabOriginHost) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_tab_origin(GURL());
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForDisallowedInitiatorUrlHost) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_initiator_url(GURL());
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(
    SearchAdsHeaderDelegateHelperTest,
    HeaderShouldNotExistForDisallowedTabOriginHostAndDisallowedInitiatorUrlHost) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_tab_origin(GURL());
  request->set_initiator_url(GURL());
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForNonSearchTabOrigin) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_tab_origin(GURL(kNonBraveSearchTabUrl));
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForNonSearchInitiatorUrl) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  request->set_initiator_url(GURL(kNonBraveSearchTabUrl));
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForNonSearchRequest) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kNonBraveSearchRequestUrl, this->profile_.get());
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForIncognito) {
  this->EnableBraveRewards();

  auto request = this->MakeRequest(
      kBraveSearchRequestUrl,
      TestingProfile::Builder().BuildIncognito(this->profile_.get()));
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForNonRewardsUser) {
  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldExistForDisconnectedRewardsUserOptedOutOfAds) {
  this->EnableBraveRewards();
  this->OptOutOfSearchResultAds();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  this->VerifyHeaderExistsExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldNotExistForDisconnectedRewardsUserOptedInToAds) {
  this->EnableBraveRewards();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  this->VerifyMissingHeaderExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldExistForConnectedRewardsUserOptedOutOfAds) {
  this->EnableBraveRewards();
  this->ConnectExternalBraveRewardsWallet();
  this->OptOutOfSearchResultAds();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  this->VerifyHeaderExistsExpectation(request);
}

TYPED_TEST(SearchAdsHeaderDelegateHelperTest,
           HeaderShouldExistForConnectedRewardsUserOptedInToAds) {
  this->EnableBraveRewards();
  this->ConnectExternalBraveRewardsWallet();

  auto request =
      this->MakeRequest(kBraveSearchRequestUrl, this->profile_.get());
  this->VerifyHeaderExistsExpectation(request);
}

}  // namespace brave
