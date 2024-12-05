/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_OBSERVER_MOCK_H_

#include <cstdint>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class TabManagerObserverMock : public TabManagerObserver {
 public:
  TabManagerObserverMock();

  TabManagerObserverMock(const TabManagerObserverMock&) = delete;
  TabManagerObserverMock& operator=(const TabManagerObserverMock&) = delete;

  ~TabManagerObserverMock() override;

  MOCK_METHOD(void, OnDidOpenNewTab, (const TabInfo& tab));

  MOCK_METHOD(void,
              OnTabDidLoad,
              (const TabInfo& tab, const int http_status_code));

  MOCK_METHOD(void, OnTabDidChangeFocus, (const int32_t tab_id));

  MOCK_METHOD(void, OnTabDidChange, (const TabInfo& tab));

  MOCK_METHOD(void,
              OnTextContentDidChange,
              (const int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               const std::string& text));

  MOCK_METHOD(void,
              OnHtmlContentDidChange,
              (const int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               const std::string& html));

  MOCK_METHOD(void, OnDidCloseTab, (const int32_t tab_id));

  MOCK_METHOD(void, OnTabDidStartPlayingMedia, (const int32_t tab_id));

  MOCK_METHOD(void, OnTabDidStopPlayingMedia, (const int32_t tab_id));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TABS_TAB_MANAGER_OBSERVER_MOCK_H_
