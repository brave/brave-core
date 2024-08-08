/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_OBSERVER_MOCK_H_

#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

struct AdHistoryItemInfo;

class AdHistoryManagerObserverMock : public AdHistoryManagerObserver {
 public:
  AdHistoryManagerObserverMock();

  AdHistoryManagerObserverMock(const AdHistoryManagerObserverMock&) = delete;
  AdHistoryManagerObserverMock& operator=(const AdHistoryManagerObserverMock&) =
      delete;

  AdHistoryManagerObserverMock(AdHistoryManagerObserverMock&&) noexcept =
      delete;
  AdHistoryManagerObserverMock& operator=(
      AdHistoryManagerObserverMock&&) noexcept = delete;

  ~AdHistoryManagerObserverMock() override;

  MOCK_METHOD(void,
              OnDidAppendAdHistoryItem,
              (const AdHistoryItemInfo& ad_history_item));

  MOCK_METHOD(void, OnDidLikeAd, (const AdHistoryItemInfo& ad_history_item));
  MOCK_METHOD(void, OnDidDislikeAd, (const AdHistoryItemInfo& ad_history_item));

  MOCK_METHOD(void,
              OnDidLikeSegment,
              (const AdHistoryItemInfo& ad_history_item));
  MOCK_METHOD(void,
              OnDidDislikeSegment,
              (const AdHistoryItemInfo& ad_history_item));

  MOCK_METHOD(void, OnDidSaveAd, (const AdHistoryItemInfo& ad_history_item));
  MOCK_METHOD(void, OnDidUnsaveAd, (const AdHistoryItemInfo& ad_history_item));

  MOCK_METHOD(void,
              OnDidMarkAdAsAppropriate,
              (const AdHistoryItemInfo& ad_history_item));
  MOCK_METHOD(void,
              OnDidMarkAdAsInappropriate,
              (const AdHistoryItemInfo& ad_history_item));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_OBSERVER_MOCK_H_
