/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/history/history_manager_observer.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace brave_ads {

class HistoryManagerObserverMock : public HistoryManagerObserver {
 public:
  HistoryManagerObserverMock();

  HistoryManagerObserverMock(const HistoryManagerObserverMock&) = delete;
  HistoryManagerObserverMock& operator=(const HistoryManagerObserverMock&) =
      delete;

  HistoryManagerObserverMock(HistoryManagerObserverMock&&) noexcept = delete;
  HistoryManagerObserverMock& operator=(HistoryManagerObserverMock&&) noexcept =
      delete;

  ~HistoryManagerObserverMock() override;

  MOCK_METHOD(void, OnDidAddHistory, (const HistoryItemInfo& history_item));

  MOCK_METHOD(void, OnDidLikeAd, (const AdContentInfo& ad_content));

  MOCK_METHOD(void, OnDidDislikeAd, (const AdContentInfo& ad_content));

  MOCK_METHOD(void, OnDidLikeCategory, (const std::string& category));

  MOCK_METHOD(void, OnDidDislikeCategory, (const std::string& category));

  MOCK_METHOD(void, OnDidSaveAd, (const AdContentInfo& ad_content));

  MOCK_METHOD(void, OnDidUnsaveAd, (const AdContentInfo& ad_content));

  MOCK_METHOD(void,
              OnDidMarkAdAsAppropriate,
              (const AdContentInfo& ad_content));

  MOCK_METHOD(void,
              OnDidMarkAdAsInappropriate,
              (const AdContentInfo& ad_content));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_MOCK_H_
