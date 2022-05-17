/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history.h"

#include <memory>

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/deprecated/client/client.h"
#include "bat/ads/internal/history/filters/date_range_history_filter.h"
#include "bat/ads/internal/history/filters/history_filter_factory.h"
#include "bat/ads/internal/history/history_util.h"
#include "bat/ads/internal/history/sorts/history_sort_factory.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {
namespace history {

HistoryInfo Get(const HistoryFilterType filter_type,
                const HistorySortType sort_type,
                const base::Time from_time,
                const base::Time to_time) {
  base::circular_deque<HistoryItemInfo> history = Client::Get()->GetHistory();

  const auto date_range_filter =
      std::make_unique<DateRangeHistoryFilter>(from_time, to_time);
  if (date_range_filter) {
    history = date_range_filter->Apply(history);
  }

  const auto filter = HistoryFilterFactory::Build(filter_type);
  if (filter) {
    history = filter->Apply(history);
  }

  const auto sort = HistorySortFactory::Build(sort_type);
  if (sort) {
    history = sort->Apply(history);
  }

  HistoryInfo normalized_history;
  for (const auto& item : history) {
    normalized_history.items.push_back(item);
  }

  return normalized_history;
}

void AddAdNotification(const AdNotificationInfo& ad,
                       const ConfirmationType& confirmation_type) {
  const HistoryItemInfo& history_item =
      BuildHistoryItem(ad, confirmation_type, ad.title, ad.body);

  Client::Get()->AppendHistory(history_item);
}

void AddNewTabPageAd(const NewTabPageAdInfo& ad,
                     const ConfirmationType& confirmation_type) {
  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, confirmation_type, ad.company_name, ad.alt);

  Client::Get()->AppendHistory(history_item);
}

void AddPromotedContentAd(const PromotedContentAdInfo& ad,
                          const ConfirmationType& confirmation_type) {
  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, confirmation_type, ad.title, ad.description);

  Client::Get()->AppendHistory(history_item);
}

void AddInlineContentAd(const InlineContentAdInfo& ad,
                        const ConfirmationType& confirmation_type) {
  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, confirmation_type, ad.title, ad.description);

  Client::Get()->AppendHistory(history_item);
}

void AddSearchResultAd(const SearchResultAdInfo& ad,
                       const ConfirmationType& confirmation_type) {
  const HistoryItemInfo history_item =
      BuildHistoryItem(ad, confirmation_type, ad.headline_text, ad.description);

  Client::Get()->AppendHistory(history_item);
}

}  // namespace history
}  // namespace ads
