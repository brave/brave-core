/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/browser/ads_service.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class MockAdsService : public AdsService {
 public:
  MockAdsService();
  ~MockAdsService() override;

  MockAdsService(const MockAdsService&) = delete;
  MockAdsService& operator=(const MockAdsService&) = delete;

  MOCK_CONST_METHOD0(IsSupportedLocale, bool());

  MOCK_CONST_METHOD0(IsEnabled, bool());
  MOCK_METHOD1(SetEnabled, void(bool));

  MOCK_METHOD1(SetAllowConversionTracking, void(bool));

  MOCK_CONST_METHOD0(GetAdsPerHour, int64_t());
  MOCK_METHOD1(SetAdsPerHour, void(int64_t));

  MOCK_CONST_METHOD0(ShouldAllowAdsSubdivisionTargeting, bool());
  MOCK_CONST_METHOD0(GetAdsSubdivisionTargetingCode, std::string());
  MOCK_METHOD1(SetAdsSubdivisionTargetingCode, void(const std::string&));
  MOCK_CONST_METHOD0(GetAutoDetectedAdsSubdivisionTargetingCode, std::string());
  MOCK_METHOD1(SetAutoDetectedAdsSubdivisionTargetingCode,
               void(const std::string&));

  MOCK_CONST_METHOD0(NeedsBrowserUpdateToSeeAds, bool());

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  MOCK_METHOD2(ShowScheduledCaptcha,
               void(const std::string&, const std::string&));
  MOCK_METHOD0(SnoozeScheduledCaptcha, void());
#endif

  MOCK_METHOD1(OnShowNotificationAd, void(const std::string&));
  MOCK_METHOD2(OnCloseNotificationAd, void(const std::string&, bool));
  MOCK_METHOD1(OnClickNotificationAd, void(const std::string&));

  MOCK_METHOD1(ChangeLocale, void(const std::string&));

  MOCK_METHOD3(OnHtmlLoaded,
               void(const SessionID&,
                    const std::vector<GURL>&,
                    const std::string&));

  MOCK_METHOD3(OnTextLoaded,
               void(const SessionID&,
                    const std::vector<GURL>&,
                    const std::string&));

  MOCK_METHOD1(OnUserGesture, void(int32_t));

  MOCK_METHOD1(OnMediaStart, void(const SessionID&));
  MOCK_METHOD1(OnMediaStop, void(const SessionID&));

  MOCK_METHOD4(OnTabUpdated, void(const SessionID&, const GURL&, bool, bool));

  MOCK_METHOD1(OnTabClosed, void(const SessionID&));

  MOCK_METHOD1(OnResourceComponentUpdated, void(const std::string&));

  MOCK_METHOD3(TriggerNewTabPageAdEvent,
               void(const std::string&,
                    const std::string&,
                    ads::mojom::NewTabPageAdEventType));
  MOCK_METHOD2(OnFailedToServeNewTabPageAd,
               void(const std::string&, const std::string&));

  MOCK_METHOD3(TriggerPromotedContentAdEvent,
               void(const std::string&,
                    const std::string&,
                    ads::mojom::PromotedContentAdEventType));

  MOCK_METHOD2(GetInlineContentAd,
               void(const std::string&, OnGetInlineContentAdCallback));
  MOCK_METHOD3(TriggerInlineContentAdEvent,
               void(const std::string&,
                    const std::string&,
                    ads::mojom::InlineContentAdEventType));

  MOCK_METHOD3(TriggerSearchResultAdEvent,
               void(ads::mojom::SearchResultAdPtr,
                    const ads::mojom::SearchResultAdEventType,
                    TriggerSearchResultAdEventCallback));

  MOCK_METHOD0(PrefetchNewTabPageAd, void());

  MOCK_METHOD0(GetPrefetchedNewTabPageAd,
               absl::optional<ads::NewTabPageAdInfo>());

  MOCK_METHOD2(PurgeOrphanedAdEventsForType,
               void(ads::mojom::AdType, PurgeOrphanedAdEventsForTypeCallback));

  MOCK_METHOD3(GetHistory, void(base::Time, base::Time, OnGetHistoryCallback));

  MOCK_METHOD1(GetStatementOfAccounts, void(GetStatementOfAccountsCallback));

  MOCK_METHOD1(GetDiagnostics, void(GetDiagnosticsCallback));

  MOCK_METHOD2(ToggleAdThumbUp,
               void(const std::string&, OnToggleAdThumbUpCallback));
  MOCK_METHOD2(ToggleAdThumbDown,
               void(const std::string&, OnToggleAdThumbDownCallback));

  MOCK_METHOD3(ToggleAdOptIn,
               void(const std::string&, int, OnToggleAdOptInCallback));
  MOCK_METHOD3(ToggleAdOptOut,
               void(const std::string&, int, OnToggleAdOptOutCallback));

  MOCK_METHOD2(ToggleSavedAd,
               void(const std::string&, OnToggleSavedAdCallback));

  MOCK_METHOD2(ToggleFlaggedAd,
               void(const std::string&, OnToggleFlaggedAdCallback));

  MOCK_METHOD1(ResetAllState, void(bool));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_
