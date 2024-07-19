/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_util.h"

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

AdHistoryItemInfo AppendAdHistoryItem(const AdInfo& ad,
                                      const ConfirmationType confirmation_type,
                                      const std::string& title,
                                      const std::string& description) {
  CHECK(ad.IsValid());

  AdHistoryItemInfo ad_history_item =
      BuildAdHistoryItem(ad, confirmation_type, title, description);
  ClientStateManager::GetInstance().AppendAdHistoryItem(ad_history_item);
  return ad_history_item;
}

}  // namespace brave_ads
