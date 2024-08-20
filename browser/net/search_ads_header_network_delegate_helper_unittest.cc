/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/search_ads_header_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_registry.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_rewards/common/features.h"
#endif  // BUILDFLAG(IS_ANDROID)

using testing::Return;

namespace {

constexpr char kBraveSearchRequestUrl[] =
    "https://search.brave.com/search?q=qwerty";
constexpr char kBraveSearchImageRequestUrl[] =
    "https://search.brave.com/img.png";
constexpr char kNonBraveSearchRequestUrl[] =
    "https://brave.com/search?q=qwerty";
constexpr char kBraveSearchTabUrl[] = "https://search.brave.com";
constexpr char kNonBraveSearchTabUrl[] = "https://brave.com";

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

  brave_l10n::test::ScopedDefaultLocale scoped_locale_{"en_US"};
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(SearchAdsHeaderDelegateHelperTest, BraveSearchTabRewardsEnabled) {
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  profile_->GetPrefs()->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds,
                                   false);

  auto request_info = std::make_shared<brave::BraveRequestInfo>();
  request_info->browser_context = profile_.get();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);

  {
    request_info->request_url = GURL(kBraveSearchTabUrl);
    request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    auto header_value = headers.GetHeader(brave::kSearchAdsHeader);
    ASSERT_TRUE(header_value);
    EXPECT_EQ(*header_value, brave::kSearchAdsDisabledValue);
  }

  {
    request_info->request_url = GURL(kBraveSearchTabUrl);
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    auto header_value = headers.GetHeader(brave::kSearchAdsHeader);
    ASSERT_TRUE(header_value);
    EXPECT_EQ(*header_value, brave::kSearchAdsDisabledValue);
  }

  {
    request_info->request_url = GURL(kBraveSearchImageRequestUrl);
    request_info->resource_type = blink::mojom::ResourceType::kImage;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    auto header_value = headers.GetHeader(brave::kSearchAdsHeader);
    ASSERT_TRUE(header_value);
    EXPECT_EQ(*header_value, brave::kSearchAdsDisabledValue);
  }

  {
    request_info->tab_origin = GURL();
    request_info->initiator_url = GURL(kBraveSearchTabUrl);
    request_info->request_url = GURL(kBraveSearchTabUrl);
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    auto header_value = headers.GetHeader(brave::kSearchAdsHeader);
    ASSERT_TRUE(header_value);
    EXPECT_EQ(*header_value, brave::kSearchAdsDisabledValue);
  }
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       NonBraveSearchTabSearchRewardsEnabled) {
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  profile_->GetPrefs()->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds,
                                   false);

  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = profile_.get();
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  {
    request_info->tab_origin = GURL(kNonBraveSearchTabUrl);
    request_info->initiator_url = GURL();

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
  }

  {
    request_info->tab_origin = GURL();
    request_info->initiator_url = GURL(kNonBraveSearchTabUrl);

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
  }
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       NonBraveSearchRequestSearchRewardsEnabled) {
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  profile_->GetPrefs()->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds,
                                   false);

  auto request_info = std::make_shared<brave::BraveRequestInfo>(
      GURL(kNonBraveSearchRequestUrl));
  request_info->browser_context = profile_.get();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);
  request_info->resource_type = blink::mojom::ResourceType::kXhr;

  net::HttpRequestHeaders headers;
  const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
      &headers, brave::ResponseCallback(), request_info);
  EXPECT_EQ(result_code, net::OK);

  EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
}

TEST_F(SearchAdsHeaderDelegateHelperTest, BraveSearchHostRewardsDisabled) {
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = profile_.get();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);

  {
    request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
  }

  {
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
  }
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       BraveSearchHostSearchResultAdsEnabled) {
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  profile_->GetPrefs()->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds,
                                   true);

  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = profile_.get();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);

  {
    request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
  }

  {
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(result_code, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
  }
}

TEST_F(SearchAdsHeaderDelegateHelperTest, BraveSearchHostIncognitoProfile) {
  TestingProfile* incognito_profile =
      TestingProfile::Builder().BuildIncognito(profile_.get());

  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = incognito_profile;
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  net::HttpRequestHeaders headers;
  const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
      &headers, brave::ResponseCallback(), request_info);
  EXPECT_EQ(result_code, net::OK);

  EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
}

TEST_F(SearchAdsHeaderDelegateHelperTest,
       BraveSearchHostIncognitoProfileWhenRewardsEnabledInMainProfile) {
  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  profile_->GetPrefs()->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds,
                                   false);

  TestingProfile* incognito_profile =
      TestingProfile::Builder().BuildIncognito(profile_.get());

  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = incognito_profile;
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  net::HttpRequestHeaders headers;
  const int result_code = brave::OnBeforeStartTransaction_SearchAdsHeader(
      &headers, brave::ResponseCallback(), request_info);
  EXPECT_EQ(result_code, net::OK);

  EXPECT_FALSE(headers.HasHeader(brave::kSearchAdsHeader));
}
