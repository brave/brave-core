/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TAB_TEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TAB_TEST_HELPER_H_

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "base/memory/raw_ref.h"

class GURL;

namespace brave_ads {

class AdsClientNotifier;

namespace test {

// Encapsulates tab lifecycle simulation for test fixtures. Each method fires
// the corresponding `AdsClientNotifier` notifications.
class TabHelper final {
 public:
  explicit TabHelper(AdsClientNotifier& ads_client_notifier);

  TabHelper(const TabHelper&) = delete;
  TabHelper& operator=(const TabHelper&) = delete;

  ~TabHelper();

  void OpenTab(int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               int http_status_code);
  void NavigateToUrl(int32_t tab_id,
                     const std::vector<GURL>& redirect_chain,
                     int http_status_code);
  void SelectTab(int32_t tab_id);
  void CloseTab(int32_t tab_id);

 private:
  void SelectLastTab();

  const raw_ref<AdsClientNotifier> ads_client_notifier_;

  std::optional<int32_t> visible_tab_id_;
  std::map</*tab_id=*/int32_t, std::vector<GURL>> redirect_chains_;
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TAB_TEST_HELPER_H_
