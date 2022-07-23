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
  MockAdsService(const MockAdsService&) = delete;
  MockAdsService& operator=(const MockAdsService&) = delete;
  ~MockAdsService() override;

  MOCK_CONST_METHOD0(IsSupportedLocale, bool());

  MOCK_CONST_METHOD0(IsEnabled, bool());
  MOCK_METHOD1(SetEnabled, void(bool));

  MOCK_CONST_METHOD0(GetNotificationAdsPerHour, int64_t());
  MOCK_METHOD1(SetNotificationAdsPerHour, void(int64_t));

  MOCK_METHOD1(SetAllowConversionTracking, void(bool));

  MOCK_CONST_METHOD0(ShouldAllowSubdivisionTargeting, bool());
  MOCK_CONST_METHOD0(GetSubdivisionTargetingCode, std::string());
  MOCK_METHOD1(SetSubdivisionTargetingCode, void(const std::string&));
  MOCK_CONST_METHOD0(GetAutoDetectedSubdivisionTargetingCode, std::string());
  MOCK_METHOD1(SetAutoDetectedSubdivisionTargetingCode,
               void(const std::string&));

  MOCK_CONST_METHOD0(NeedsBrowserUpgradeToServeAds, bool());

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  MOCK_METHOD2(ShowScheduledCaptcha,
               void(const std::string&, const std::string&));
  MOCK_METHOD0(SnoozeScheduledCaptcha, void());
#endif

  MOCK_METHOD1(OnNotificationAdShown, void(const std::string&));
  MOCK_METHOD2(OnNotificationAdClosed, void(const std::string&, bool));
  MOCK_METHOD1(OnNotificationAdClicked, void(const std::string&));

  MOCK_METHOD1(GetDiagnostics, void(GetDiagnosticsCallback));

  MOCK_METHOD1(OnChangeLocale, void(const std::string&));

  MOCK_METHOD1(OnResourceComponentUpdated, void(const std::string&));

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

  MOCK_METHOD1(GetStatementOfAccounts, void(GetStatementOfAccountsCallback));

  MOCK_METHOD2(MaybeServeInlineContentAd,
               void(const std::string&, MaybeServeInlineContentAdCallback));
  MOCK_METHOD3(TriggerInlineContentAdEvent,
               void(const std::string&,
                    const std::string&,
                    ads::mojom::InlineContentAdEventType));

  MOCK_METHOD0(GetPrefetchedNewTabPageAd,
               absl::optional<ads::NewTabPageAdInfo>());
  MOCK_METHOD0(PrefetchNewTabPageAd, void());
  MOCK_METHOD3(TriggerNewTabPageAdEvent,
               void(const std::string&,
                    const std::string&,
                    ads::mojom::NewTabPageAdEventType));
  MOCK_METHOD2(OnFailedToPrefetchNewTabPageAd,
               void(const std::string&, const std::string&));

  MOCK_METHOD3(TriggerPromotedContentAdEvent,
               void(const std::string&,
                    const std::string&,
                    ads::mojom::PromotedContentAdEventType));

  MOCK_METHOD3(TriggerSearchResultAdEvent,
               void(ads::mojom::SearchResultAdPtr,
                    const ads::mojom::SearchResultAdEventType,
                    TriggerSearchResultAdEventCallback));

  MOCK_METHOD2(PurgeOrphanedAdEventsForType,
               void(ads::mojom::AdType, PurgeOrphanedAdEventsForTypeCallback));

  MOCK_METHOD3(GetHistory, void(base::Time, base::Time, GetHistoryCallback));

  MOCK_METHOD2(ToggleAdThumbUp,
               void(const std::string&, ToggleAdThumbUpCallback));
  MOCK_METHOD2(ToggleAdThumbDown,
               void(const std::string&, ToggleAdThumbDownCallback));
  MOCK_METHOD3(ToggleAdOptIn,
               void(const std::string&, int, ToggleAdOptInCallback));
  MOCK_METHOD3(ToggleAdOptOut,
               void(const std::string&, int, ToggleAdOptOutCallback));
  MOCK_METHOD2(ToggleSavedAd, void(const std::string&, ToggleSavedAdCallback));
  MOCK_METHOD2(ToggleFlaggedAd,
               void(const std::string&, ToggleFlaggedAdCallback));

  MOCK_METHOD1(WipeState, void(bool));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_
