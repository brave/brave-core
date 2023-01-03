/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_util.h"

#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/history/history_item_util.h"

namespace ads {

HistoryItemInfo AddHistory(const AdInfo& ad,
                           const ConfirmationType& confirmation_type,
                           const std::string& title,
                           const std::string& description) {
  HistoryItemInfo history_item =
      BuildHistoryItem(ad, confirmation_type, title, description);
  ClientStateManager::GetInstance()->AppendHistory(history_item);
  return history_item;
}

}  // namespace ads
