/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {

constexpr char kPlacementId[] = "15c6eecc-b8c2-4033-924e-26a12500e7be";
constexpr char kCreativeInstanceId[] = "7e352bd8-affc-4d47-90d8-316480152bd8";

}  // namespace

class NTPSponsoredRichMediaAdEventHandlerTest : public testing::Test {
 protected:
  void VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
      bool should_record) {
    std::unique_ptr<NTPP3AHelperMock> ntp_p3a_helper =
        std::make_unique<NTPP3AHelperMock>();
    raw_ptr<NTPP3AHelperMock> ntp_p3a_helper_ptr = ntp_p3a_helper.get();

    NTPSponsoredRichMediaAdEventHandler ad_event_handler(
        /*ads_service=*/nullptr, std::move(ntp_p3a_helper));

    if (should_record) {
      EXPECT_CALL(
          *ntp_p3a_helper_ptr,
          RecordNewTabPageAdEvent(mojom_ad_event_type, kCreativeInstanceId));
    } else {
      EXPECT_CALL(*ntp_p3a_helper_ptr, RecordNewTabPageAdEvent).Times(0);
    }
    ad_event_handler.ReportRichMediaAdEvent(kPlacementId, kCreativeInstanceId,
                                            mojom_ad_event_type);

    // Reset the `ntp_p3a_helper_ptr` to nullptr to avoid dangling pointer.
    ntp_p3a_helper_ptr = nullptr;
  }

  void VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
      bool should_trigger) {
    brave_ads::AdsServiceMock ads_service(/*delegate=*/nullptr);

    NTPSponsoredRichMediaAdEventHandler ad_event_handler(
        &ads_service, /*ntp_p3a_helper=*/nullptr);

    if (should_trigger) {
      EXPECT_CALL(ads_service,
                  TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                           mojom_ad_event_type,
                                           /*callback=*/::testing::_));
    } else {
      EXPECT_CALL(ads_service, TriggerNewTabPageAdEvent).Times(0);
    }
    ad_event_handler.ReportRichMediaAdEvent(kPlacementId, kCreativeInstanceId,
                                            mojom_ad_event_type);
  }
};

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest, RecordNewTabPageAdEvent) {
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kClicked,
      /*should_record=*/true);
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kInteraction,
      /*should_record=*/true);
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kMediaPlay,
      /*should_record=*/true);
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia25,
      /*should_record=*/true);
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia100,
      /*should_record=*/true);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest, DoNotRecordNewTabPageAdEvent) {
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kServedImpression,
      /*should_record=*/false);
  VerifyRecordNewTabPageAdEventExpecation(
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_record=*/false);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest, TriggerNewTabPageAdEvent) {
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kClicked,
      /*should_trigger=*/true);
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kInteraction,
      /*should_trigger=*/true);
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMediaPlay,
      /*should_trigger=*/true);
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia25,
      /*should_trigger=*/true);
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia100,
      /*should_trigger=*/true);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest, DoNotTriggerNewTabPageAdEvent) {
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kServedImpression,
      /*should_trigger=*/false);
  VerifyTriggerNewTabPageAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_trigger=*/false);
}

}  // namespace ntp_background_images
