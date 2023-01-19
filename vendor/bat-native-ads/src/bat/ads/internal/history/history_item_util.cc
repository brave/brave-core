/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_item_util.h"

#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/history/ad_content_util.h"
#include "bat/ads/internal/history/category_content_util.h"

namespace ads {

HistoryItemInfo BuildHistoryItem(const AdInfo& ad,
                                 const ConfirmationType& confirmation_type,
                                 const std::string& title,
                                 const std::string& description) {
  HistoryItemInfo history_item;

  history_item.created_at = base::Time::Now();
  history_item.ad_content =
      BuildAdContent(ad, confirmation_type, title, description);
  history_item.category_content = BuildCategoryContent(ad.segment);

  return history_item;
}

}  // namespace ads
