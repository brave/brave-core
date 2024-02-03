/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_MOCK_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace brave_ads {

class AdsServiceMock : public AdsService {
 public:
  AdsServiceMock();

  AdsServiceMock(const AdsServiceMock&) = delete;
  AdsServiceMock& operator=(const AdsServiceMock&) = delete;

  AdsServiceMock(AdsServiceMock&&) noexcept = delete;
  AdsServiceMock& operator=(AdsServiceMock&&) noexcept = delete;

  ~AdsServiceMock() override;

  MOCK_METHOD(void,
              AddBatAdsObserver,
              (mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> observer));

  MOCK_METHOD(int64_t, GetMaximumNotificationAdsPerHour, (), (const));

  MOCK_METHOD(void,
              ShowScheduledCaptcha,
              (const std::string&, const std::string&));
  MOCK_METHOD(void, SnoozeScheduledCaptcha, ());

  MOCK_METHOD(void, OnNotificationAdShown, (const std::string&));
  MOCK_METHOD(void, OnNotificationAdClosed, (const std::string&, bool));
  MOCK_METHOD(void, OnNotificationAdClicked, (const std::string&));

  MOCK_METHOD(void, GetDiagnostics, (GetDiagnosticsCallback));

  MOCK_METHOD(void, GetStatementOfAccounts, (GetStatementOfAccountsCallback));

  MOCK_METHOD(bool, IsBrowserUpgradeRequiredToServeAds, (), (const));

  MOCK_METHOD(void,
              MaybeServeInlineContentAd,
              (const std::string&, MaybeServeInlineContentAdAsDictCallback));
  MOCK_METHOD(void,
              TriggerInlineContentAdEvent,
              (const std::string&,
               const std::string&,
               mojom::InlineContentAdEventType,
               TriggerAdEventCallback));

  MOCK_METHOD(std::optional<NewTabPageAdInfo>,
              GetPrefetchedNewTabPageAdForDisplay,
              ());
  MOCK_METHOD(void, PrefetchNewTabPageAd, ());
  MOCK_METHOD(void,
              TriggerNewTabPageAdEvent,
              (const std::string&,
               const std::string&,
               mojom::NewTabPageAdEventType,
               TriggerAdEventCallback));
  MOCK_METHOD(void,
              OnFailedToPrefetchNewTabPageAd,
              (const std::string&, const std::string&));

  MOCK_METHOD(void,
              TriggerPromotedContentAdEvent,
              (const std::string&,
               const std::string&,
               mojom::PromotedContentAdEventType,
               TriggerAdEventCallback));

  MOCK_METHOD(void,
              TriggerSearchResultAdEvent,
              (mojom::SearchResultAdInfoPtr,
               const mojom::SearchResultAdEventType,
               TriggerAdEventCallback));

  MOCK_METHOD(void,
              PurgeOrphanedAdEventsForType,
              (mojom::AdType, PurgeOrphanedAdEventsForTypeCallback));

  MOCK_METHOD(void, GetHistory, (base::Time, base::Time, GetHistoryCallback));

  MOCK_METHOD(void, ToggleLikeAd, (base::Value::Dict, ToggleLikeAdCallback));
  MOCK_METHOD(void,
              ToggleDislikeAd,
              (base::Value::Dict, ToggleDislikeAdCallback));
  MOCK_METHOD(void,
              ToggleLikeCategory,
              (base::Value::Dict, ToggleLikeCategoryCallback));
  MOCK_METHOD(void,
              ToggleDislikeCategory,
              (base::Value::Dict, ToggleDislikeCategoryCallback));
  MOCK_METHOD(void, ToggleSaveAd, (base::Value::Dict, ToggleSaveAdCallback));
  MOCK_METHOD(void,
              ToggleMarkAdAsInappropriate,
              (base::Value::Dict, ToggleMarkAdAsInappropriateCallback));

  MOCK_METHOD(void,
              NotifyTabTextContentDidChange,
              (int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               const std::string& text));
  MOCK_METHOD(void,
              NotifyTabHtmlContentDidChange,
              (int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               const std::string& html));
  MOCK_METHOD(void, NotifyTabDidStartPlayingMedia, (int32_t tab_id));
  MOCK_METHOD(void, NotifyTabDidStopPlayingMedia, (int32_t tab_id));
  MOCK_METHOD(void,
              NotifyTabDidChange,
              (int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               bool is_visible));
  MOCK_METHOD(void, NotifyDidCloseTab, (int32_t tab_id));
  MOCK_METHOD(void, NotifyUserGestureEventTriggered, (int32_t tab_id));
  MOCK_METHOD(void, NotifyBrowserDidBecomeActive, ());
  MOCK_METHOD(void, NotifyBrowserDidResignActive, ());

  MOCK_METHOD(void, NotifyDidSolveAdaptiveCaptcha, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_MOCK_H_
