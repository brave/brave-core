/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_OBSERVER_H_

#include "base/observer_list_types.h"

namespace brave_ads {

struct AdHistoryItemInfo;

class AdHistoryManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when ad history is added.
  virtual void OnDidAddAdHistoryItem(const AdHistoryItemInfo& ad_history_item) {
  }
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_MANAGER_OBSERVER_H_
