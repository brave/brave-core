/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"

#include <memory>

#include "brave/components/brave_ads/core/browser/service/ads_service_mock.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {

constexpr char kPlacementId[] = "15c6eecc-b8c2-4033-924e-26a12500e7be";
constexpr char kCreativeInstanceId[] = "7e352bd8-affc-4d47-90d8-316480152bd8";

}  // namespace

class NTPSponsoredRichMediaAdEventHandlerTest : public testing::Test {
 protected:
  void VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
      bool should_metrics_fallback_to_p3a,
      bool should_report) {
    brave_ads::AdsServiceMock ads_service;

    std::unique_ptr<NTPP3AHelperMock> ntp_p3a_helper =
        std::make_unique<NTPP3AHelperMock>();

    NTPSponsoredRichMediaAdEventHandler ad_event_handler(&ads_service,
                                                         ntp_p3a_helper.get());

    if (should_metrics_fallback_to_p3a) {
      if (should_report) {
        EXPECT_CALL(
            *ntp_p3a_helper,
            RecordNewTabPageAdEvent(mojom_ad_event_type, kCreativeInstanceId));

        // `TriggerNewTabPageAdEvent` should be called because the ads service
        // will handle the case when we should fallback to P3A and no-op if the
        // campaign should report using P3A.
        EXPECT_CALL(ads_service, TriggerNewTabPageAdEvent);
      } else {
        EXPECT_CALL(*ntp_p3a_helper, RecordNewTabPageAdEvent).Times(0);
        EXPECT_CALL(ads_service, TriggerNewTabPageAdEvent).Times(0);
      }
    } else {
      if (should_report) {
        EXPECT_CALL(ads_service,
                    TriggerNewTabPageAdEvent(kPlacementId, kCreativeInstanceId,
                                             should_metrics_fallback_to_p3a,
                                             mojom_ad_event_type,
                                             /*callback=*/::testing::_));
      } else {
        EXPECT_CALL(ads_service, TriggerNewTabPageAdEvent).Times(0);
      }
      EXPECT_CALL(*ntp_p3a_helper, RecordNewTabPageAdEvent).Times(0);
    }

    ad_event_handler.MaybeReportRichMediaAdEvent(
        kPlacementId, kCreativeInstanceId, should_metrics_fallback_to_p3a,
        mojom_ad_event_type);
  }
};

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest, ReportAdEventMetricUsingP3a) {
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kClicked,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kInteraction,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMediaPlay,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia25,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia100,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/true);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       DoNotReportAdEventMetricUsingP3a) {
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kServedImpression,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/false);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_metrics_fallback_to_p3a=*/true,
      /*should_report=*/false);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       ReportAdEventMetricUsingConfirmation) {
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kClicked,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kInteraction,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMediaPlay,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia25,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/true);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia100,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/true);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       DoNotReportAdEventMetricUsingConfirmation) {
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kServedImpression,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/false);
  VerifyReportAdEventMetricExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression,
      /*should_metrics_fallback_to_p3a=*/false,
      /*should_report=*/false);
}

}  // namespace ntp_background_images
