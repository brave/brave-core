/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/browser/ads_service.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

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

  MOCK_CONST_METHOD0(GetMaximumNotificationAdsPerHour, int64_t());
  MOCK_METHOD1(SetMaximumNotificationAdsPerHour, void(int64_t));

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

  MOCK_METHOD1(OnLocaleDidChange, void(const std::string&));

  MOCK_METHOD1(OnDidUpdateResourceComponent, void(const std::string&));

  MOCK_METHOD3(OnTabHtmlContentDidChange,
               void(const SessionID&,
                    const std::vector<GURL>&,
                    const std::string&));
  MOCK_METHOD3(OnTabTextContentDidChange,
               void(const SessionID&,
                    const std::vector<GURL>&,
                    const std::string&));

  MOCK_METHOD1(TriggerUserGestureEvent, void(int32_t));

  MOCK_METHOD1(OnTabDidStartPlayingMedia, void(const SessionID&));
  MOCK_METHOD1(OnTabDidStopPlayingMedia, void(const SessionID&));
  MOCK_METHOD4(OnTabDidChange,
               void(const SessionID&, const std::vector<GURL>&, bool, bool));
  MOCK_METHOD1(OnDidCloseTab, void(const SessionID&));

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

  MOCK_METHOD2(TriggerSearchResultAdEvent,
               void(ads::mojom::SearchResultAdInfoPtr,
                    const ads::mojom::SearchResultAdEventType));

  MOCK_METHOD2(PurgeOrphanedAdEventsForType,
               void(ads::mojom::AdType, PurgeOrphanedAdEventsForTypeCallback));

  MOCK_METHOD3(GetHistory, void(base::Time, base::Time, GetHistoryCallback));

  MOCK_METHOD2(ToggleAdThumbUp,
               void(base::Value::Dict, ToggleAdThumbUpCallback));
  MOCK_METHOD2(ToggleAdThumbDown,
               void(base::Value::Dict, ToggleAdThumbDownCallback));
  MOCK_METHOD3(ToggleAdOptIn,
               void(const std::string&, int, ToggleAdOptInCallback));
  MOCK_METHOD3(ToggleAdOptOut,
               void(const std::string&, int, ToggleAdOptOutCallback));
  MOCK_METHOD2(ToggleSavedAd, void(base::Value::Dict, ToggleSavedAdCallback));
  MOCK_METHOD2(ToggleFlaggedAd,
               void(base::Value::Dict, ToggleFlaggedAdCallback));

  MOCK_METHOD1(WipeState, void(bool));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_MOCK_ADS_SERVICE_H_
