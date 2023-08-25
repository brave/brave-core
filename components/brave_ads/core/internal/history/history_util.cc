/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_util.h"

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_item_util.h"
#include "brave/components/brave_ads/core/public/ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"

namespace brave_ads {

HistoryItemInfo AddHistory(const AdInfo& ad,
                           const ConfirmationType& confirmation_type,
                           const std::string& title,
                           const std::string& description) {
  CHECK(ad.IsValid());

  HistoryItemInfo history_item =
      BuildHistoryItem(ad, confirmation_type, title, description);
  ClientStateManager::GetInstance().AppendHistory(history_item);
  return history_item;
}

}  // namespace brave_ads
