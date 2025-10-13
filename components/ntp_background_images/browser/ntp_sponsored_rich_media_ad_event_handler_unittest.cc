/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"

#include <memory>

#include "brave/components/brave_ads/core/browser/service/ads_service_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {

constexpr char kPlacementId[] = "15c6eecc-b8c2-4033-924e-26a12500e7be";
constexpr char kCreativeInstanceId[] = "7e352bd8-affc-4d47-90d8-316480152bd8";

}  // namespace

class NTPSponsoredRichMediaAdEventHandlerTest : public testing::Test {
 protected:
  void VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
      brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
      bool should_report) {
    brave_ads::AdsServiceMock ads_service;

    std::unique_ptr<NTPP3AHelperMock> ntp_p3a_helper =
        std::make_unique<NTPP3AHelperMock>();

    NTPSponsoredRichMediaAdEventHandler ad_event_handler(&ads_service,
                                                         ntp_p3a_helper.get());

    if (should_report) {
      EXPECT_CALL(ads_service, TriggerNewTabPageAdEvent(
                                   kPlacementId, kCreativeInstanceId,
                                   mojom_ad_metric_type, mojom_ad_event_type,
                                   /*callback=*/::testing::_));

      if (mojom_ad_metric_type ==
          brave_ads::mojom::NewTabPageAdMetricType::kP3A) {
        EXPECT_CALL(
            *ntp_p3a_helper,
            RecordNewTabPageAdEvent(mojom_ad_event_type, kCreativeInstanceId));
      } else {
        EXPECT_CALL(*ntp_p3a_helper, RecordNewTabPageAdEvent).Times(0);
      }
    } else {
      EXPECT_CALL(*ntp_p3a_helper, RecordNewTabPageAdEvent).Times(0);
      EXPECT_CALL(ads_service, TriggerNewTabPageAdEvent).Times(0);
    }

    ad_event_handler.MaybeReportRichMediaAdEvent(
        kPlacementId, kCreativeInstanceId, mojom_ad_metric_type,
        mojom_ad_event_type);
  }
};

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       ReportAdEventWhenMetricTypeIsP3A) {
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kClicked,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kInteraction,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMediaPlay,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia25,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia100,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/true);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       DoNotReportAdEventWhenMetricTypeIsP3a) {
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kServedImpression,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/false);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression,
      brave_ads::mojom::NewTabPageAdMetricType::kP3A,
      /*should_report=*/false);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       ReportAdEventWhenMetricTypeIsConfirmation) {
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kClicked,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kInteraction,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMediaPlay,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia25,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/true);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kMedia100,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/true);
}

TEST_F(NTPSponsoredRichMediaAdEventHandlerTest,
       DoNotReportAdEventWhenMetricTypeIsConfirmation) {
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kServedImpression,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/false);
  VerifyReportAdEventExpectation(
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression,
      brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
      /*should_report=*/false);
}

}  // namespace ntp_background_images
