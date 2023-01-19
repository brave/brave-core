/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/net/brave_ads_status_header_network_delegate_helper.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/mock_ads_service.h"
#include "chrome/test/base/testing_profile.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

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

class AdsStatusHeaderDelegateHelperTest : public testing::Test {
 public:
  brave_ads::MockAdsService* SetUpAdsService(TestingProfile* profile) {
    KeyedService* ads_service =
        brave_ads::AdsServiceFactory::GetInstance()->SetTestingFactoryAndUse(
            profile, base::BindRepeating([](content::BrowserContext* context)
                                             -> std::unique_ptr<KeyedService> {
              if (context->IsOffTheRecord()) {
                return {};
              }
              return std::make_unique<brave_ads::MockAdsService>();
            }));
    return static_cast<brave_ads::MockAdsService*>(ads_service);
  }

  TestingProfile* profile() { return &profile_; }

  brave_ads::MockAdsService* ads_service() { return ads_service_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  brave_ads::MockAdsService* ads_service_;
  TestingProfile profile_;
};

TEST_F(AdsStatusHeaderDelegateHelperTest, BraveSearchTabAdsEnabled) {
  auto request_info = std::make_shared<brave::BraveRequestInfo>();
  request_info->browser_context = profile();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  brave_ads::MockAdsService* ads_service = SetUpAdsService(profile());
  ASSERT_TRUE(ads_service);
  EXPECT_CALL(*ads_service, IsEnabled()).WillRepeatedly(Return(true));

  {
    request_info->request_url = GURL(kBraveSearchTabUrl);
    request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    std::string ads_status_header;
    EXPECT_TRUE(headers.GetHeader(brave::kAdsStatusHeader, &ads_status_header));
    EXPECT_EQ(ads_status_header, brave::kAdsEnabledStatusValue);
  }

  {
    request_info->request_url = GURL(kBraveSearchTabUrl);
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    std::string ads_status_header;
    EXPECT_TRUE(headers.GetHeader(brave::kAdsStatusHeader, &ads_status_header));
    EXPECT_EQ(ads_status_header, brave::kAdsEnabledStatusValue);
  }

  {
    request_info->request_url = GURL(kBraveSearchImageRequestUrl);
    request_info->resource_type = blink::mojom::ResourceType::kImage;

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    std::string ads_status_header;
    EXPECT_TRUE(headers.GetHeader(brave::kAdsStatusHeader, &ads_status_header));
    EXPECT_EQ(ads_status_header, brave::kAdsEnabledStatusValue);
  }

  {
    request_info->tab_origin = GURL();
    request_info->initiator_url = GURL(kBraveSearchTabUrl);
    request_info->request_url = GURL(kBraveSearchTabUrl);
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    std::string ads_status_header;
    EXPECT_TRUE(headers.GetHeader(brave::kAdsStatusHeader, &ads_status_header));
    EXPECT_EQ(ads_status_header, brave::kAdsEnabledStatusValue);
  }
}

TEST_F(AdsStatusHeaderDelegateHelperTest, NonBraveSearchTabAdsEnabled) {
  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = profile();
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

  brave_ads::MockAdsService* ads_service = SetUpAdsService(profile());
  ASSERT_TRUE(ads_service);
  EXPECT_CALL(*ads_service, IsEnabled()).WillRepeatedly(Return(true));

  {
    request_info->tab_origin = GURL(kNonBraveSearchTabUrl);
    request_info->initiator_url = GURL();

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kAdsStatusHeader));
  }

  {
    request_info->tab_origin = GURL();
    request_info->initiator_url = GURL(kNonBraveSearchTabUrl);

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kAdsStatusHeader));
  }
}

TEST_F(AdsStatusHeaderDelegateHelperTest, NonBraveSearchRequestAdsEnabled) {
  auto request_info = std::make_shared<brave::BraveRequestInfo>(
      GURL(kNonBraveSearchRequestUrl));
  request_info->browser_context = profile();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);
  request_info->resource_type = blink::mojom::ResourceType::kXhr;
  brave_ads::MockAdsService* ads_service = SetUpAdsService(profile());
  ASSERT_TRUE(ads_service);
  EXPECT_CALL(*ads_service, IsEnabled()).WillOnce(Return(true));

  net::HttpRequestHeaders headers;
  const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
      &headers, brave::ResponseCallback(), request_info);
  EXPECT_EQ(rc, net::OK);

  EXPECT_FALSE(headers.HasHeader(brave::kAdsStatusHeader));
}

TEST_F(AdsStatusHeaderDelegateHelperTest, BraveSearchHostAdsDisabled) {
  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = profile();
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);
  brave_ads::MockAdsService* ads_service = SetUpAdsService(profile());
  ASSERT_TRUE(ads_service);
  EXPECT_CALL(*ads_service, IsEnabled()).WillRepeatedly(Return(false));

  {
    request_info->resource_type = blink::mojom::ResourceType::kMainFrame;

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kAdsStatusHeader));
  }

  {
    request_info->resource_type = blink::mojom::ResourceType::kXhr;

    net::HttpRequestHeaders headers;
    const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
        &headers, brave::ResponseCallback(), request_info);
    EXPECT_EQ(rc, net::OK);

    EXPECT_FALSE(headers.HasHeader(brave::kAdsStatusHeader));
  }
}

TEST_F(AdsStatusHeaderDelegateHelperTest, BraveSearchHostIncognitoProfile) {
  TestingProfile* incognito_profile =
      TestingProfile::Builder().BuildIncognito(profile());
  auto request_info =
      std::make_shared<brave::BraveRequestInfo>(GURL(kBraveSearchRequestUrl));
  request_info->browser_context = incognito_profile;
  request_info->tab_origin = GURL(kBraveSearchTabUrl);
  request_info->initiator_url = GURL(kBraveSearchTabUrl);
  request_info->resource_type = blink::mojom::ResourceType::kMainFrame;
  brave_ads::MockAdsService* ads_service = SetUpAdsService(incognito_profile);
  EXPECT_FALSE(ads_service);

  net::HttpRequestHeaders headers;
  const int rc = brave::OnBeforeStartTransaction_AdsStatusHeader(
      &headers, brave::ResponseCallback(), request_info);
  EXPECT_EQ(rc, net::OK);

  EXPECT_FALSE(headers.HasHeader(brave::kAdsStatusHeader));
}
