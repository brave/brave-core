/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/search_ads_header_network_delegate_helper.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/test/scoped_feature_list.h"
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

}  // namespace

class SearchAdsHeaderDelegateHelperTest : public testing::Test {
 protected:
  void SetUp() override {
    scoped_feature_list_.InitWithFeatures(
        {
#if BUILDFLAG(IS_ANDROID)
            brave_rewards::features::kBraveRewards
#endif  // BUILDFLAG(IS_ANDROID)
        },
        {});

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

  std::shared_ptr<BraveRequestInfo> MakeRequest(std::string_view url,
                                                TestingProfile* profile) {
    auto request = std::make_shared<BraveRequestInfo>(GURL(url));
    request->request_url = GURL(kBraveSearchTabUrl);
    request->tab_origin = GURL(kBraveSearchTabUrl);
    request->initiator_url = GURL(kBraveSearchTabUrl);
    request->resource_type = blink::mojom::ResourceType::kMainFrame;
    request->browser_context = profile;
    return request;
  }

  void VerifyHeaderExistsExpectation(
      std::shared_ptr<BraveRequestInfo> request) {
    net::HttpRequestHeaders request_headers;
    const int result_code = OnBeforeStartTransaction_SearchAdsHeader(
        &request_headers, ResponseCallback(), request);
    EXPECT_EQ(result_code, net::OK);
    EXPECT_TRUE(request_headers.HasHeader(kSearchAdsHeader));
    EXPECT_EQ(request_headers.GetHeader(kSearchAdsHeader),
              kSearchAdsDisabledValue);
  }

  void VerifyMissingHeaderExpectation(
      std::shared_ptr<BraveRequestInfo> request) {
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
};

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldExistForMainFrameResource) {
  EnableBraveRewards();
  OptOutOfSearchResultAds();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->resource_type = blink::mojom::ResourceType::kMainFrame;
  VerifyHeaderExistsExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest, HeaderShouldExistForXhrResource) {
  EnableBraveRewards();
  OptOutOfSearchResultAds();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->resource_type = blink::mojom::ResourceType::kXhr;
  VerifyHeaderExistsExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest, HeaderShouldExistForImageResource) {
  EnableBraveRewards();
  OptOutOfSearchResultAds();

  auto request = MakeRequest(kBraveSearchImageRequestUrl, profile_.get());
  request->resource_type = blink::mojom::ResourceType::kImage;
  VerifyHeaderExistsExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForDisallowedTabOriginHost) {
  EnableBraveRewards();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->tab_origin = GURL();
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForDisallowedInitiatorUrlHost) {
  EnableBraveRewards();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->initiator_url = GURL();
  VerifyMissingHeaderExpectation(request);
}

TEST_F(
    SearchAdsHeaderDelegateHelperTest,
    HeaderShouldNotExistForDisallowedTabOriginHostAndDisallowedInitiatorUrlHost) {
  EnableBraveRewards();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->tab_origin = GURL();
  request->initiator_url = GURL();
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForNonSearchTabOrigin) {
  EnableBraveRewards();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->tab_origin = GURL(kNonBraveSearchTabUrl);
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForNonSearchInitiatorUrl) {
  EnableBraveRewards();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  request->initiator_url = GURL(kNonBraveSearchTabUrl);
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForNonSearchRequest) {
  EnableBraveRewards();

  auto request = MakeRequest(kNonBraveSearchRequestUrl, profile_.get());
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest, HeaderShouldNotExistForIncognito) {
  EnableBraveRewards();

  auto request =
      MakeRequest(kBraveSearchRequestUrl,
                  TestingProfile::Builder().BuildIncognito(profile_.get()));
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForNonRewardsUser) {
  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldExistForDisconnectedRewardsUserOptedOutOfAds) {
  EnableBraveRewards();
  OptOutOfSearchResultAds();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  VerifyHeaderExistsExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldNotExistForDisconnectedRewardsUserOptedInToAds) {
  EnableBraveRewards();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  VerifyMissingHeaderExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldExistForConnectedRewardsUserOptedOutOfAds) {
  EnableBraveRewards();
  ConnectExternalBraveRewardsWallet();
  OptOutOfSearchResultAds();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  VerifyHeaderExistsExpectation(request);
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       HeaderShouldExistForConnectedRewardsUserOptedInToAds) {
  EnableBraveRewards();
  ConnectExternalBraveRewardsWallet();

  auto request = MakeRequest(kBraveSearchRequestUrl, profile_.get());
  VerifyHeaderExistsExpectation(request);
}

}  // namespace brave
