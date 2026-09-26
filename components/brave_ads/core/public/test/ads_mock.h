/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TEST_ADS_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TEST_ADS_MOCK_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsMock : public Ads {
 public:
  AdsMock();

  AdsMock(const AdsMock&) = delete;
  AdsMock& operator=(const AdsMock&) = delete;

  ~AdsMock() override;

  base::WeakPtr<AdsMock> GetWeakPtr();

  MOCK_METHOD(void,
              AddObserver,
              (std::unique_ptr<AdsObserver>),
              (override));
  MOCK_METHOD(void, SetSysInfo, (mojom::SysInfoPtr), (override));
  MOCK_METHOD(void,
              SetBuildChannel,
              (mojom::BuildChannelInfoPtr),
              (override));
  MOCK_METHOD(void,
              SetCommandLineSwitches,
              (mojom::CommandLineSwitchesPtr),
              (override));
  MOCK_METHOD(void,
              SetContentSettings,
              (mojom::ContentSettingsPtr),
              (override));
  MOCK_METHOD(void,
              Initialize,
              (mojom::WalletInfoPtr, ResultCallback),
              (override));
  MOCK_METHOD(void, Shutdown, (ResultCallback), (override));
  MOCK_METHOD(void, GetInternals, (GetInternalsCallback), (override));
  MOCK_METHOD(void, GetDiagnostics, (GetDiagnosticsCallback), (override));
  MOCK_METHOD(void,
              EvaluateConditionMatcher,
              (const std::string&,
               const std::string&,
               std::optional<std::string>,
               EvaluateConditionMatcherCallback),
              (override));
  MOCK_METHOD(void,
              GetStatementOfAccounts,
              (GetStatementOfAccountsCallback),
              (override));
  MOCK_METHOD(void,
              ParseAndSaveNewTabPageAds,
              (base::DictValue, ResultCallback),
              (override));
  MOCK_METHOD(void,
              MaybeServeNewTabPageAd,
              (MaybeServeNewTabPageAdCallback),
              (override));
  MOCK_METHOD(void,
              TriggerNewTabPageAdEvent,
              (const std::string&,
               const std::string&,
               mojom::NewTabPageAdMetricType,
               mojom::NewTabPageAdEventType,
               ResultCallback),
              (override));
  MOCK_METHOD(void,
              MaybeGetNotificationAd,
              (const std::string&, MaybeGetNotificationAdCallback),
              (override));
  MOCK_METHOD(void,
              TriggerNotificationAdEvent,
              (const std::string&,
               mojom::NotificationAdEventType,
               ResultCallback),
              (override));
  MOCK_METHOD(void,
              MaybeGetSearchResultAd,
              (const std::string&, MaybeGetSearchResultAdCallback),
              (override));
  MOCK_METHOD(void,
              TriggerSearchResultAdEvent,
              (mojom::CreativeSearchResultAdInfoPtr,
               mojom::SearchResultAdEventType,
               ResultCallback),
              (override));
  MOCK_METHOD(void,
              PurgeOrphanedAdEventsForType,
              (mojom::AdType, ResultCallback),
              (override));
  MOCK_METHOD(void,
              GetAdHistory,
              (base::Time, base::Time, GetAdHistoryForUICallback),
              (override));
  MOCK_METHOD(void,
              ToggleLikeAd,
              (mojom::ReactionInfoPtr, ResultCallback),
              (override));
  MOCK_METHOD(void,
              ToggleDislikeAd,
              (mojom::ReactionInfoPtr, ResultCallback),
              (override));
  MOCK_METHOD(void,
              ToggleLikeSegment,
              (mojom::ReactionInfoPtr, ResultCallback),
              (override));
  MOCK_METHOD(void,
              ToggleDislikeSegment,
              (mojom::ReactionInfoPtr, ResultCallback),
              (override));
  MOCK_METHOD(void,
              ToggleSaveAd,
              (mojom::ReactionInfoPtr, ResultCallback),
              (override));
  MOCK_METHOD(void,
              ToggleMarkAdAsInappropriate,
              (mojom::ReactionInfoPtr, ResultCallback),
              (override));

 private:
  base::WeakPtrFactory<AdsMock> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TEST_ADS_MOCK_H_
