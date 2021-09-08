/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history.h"

#include <deque>
#include <memory>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads_history/ads_history_util.h"
#include "bat/ads/internal/ads_history/filters/ads_history_date_range_filter.h"
#include "bat/ads/internal/ads_history/filters/ads_history_filter.h"
#include "bat/ads/internal/ads_history/filters/ads_history_filter_factory.h"
#include "bat/ads/internal/ads_history/sorts/ads_history_sort.h"
#include "bat/ads/internal/ads_history/sorts/ads_history_sort_factory.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {
namespace history {

AdsHistoryInfo Get(const AdsHistoryInfo::FilterType filter_type,
                   const AdsHistoryInfo::SortType sort_type,
                   const uint64_t from_timestamp,
                   const uint64_t to_timestamp) {
  std::deque<AdHistoryInfo> ads_history = Client::Get()->GetAdsHistory();

  const auto date_range_filter = std::make_unique<AdsHistoryDateRangeFilter>();
  if (date_range_filter) {
    ads_history =
        date_range_filter->Apply(ads_history, from_timestamp, to_timestamp);
  }

  const auto filter = AdsHistoryFilterFactory::Build(filter_type);
  if (filter) {
    ads_history = filter->Apply(ads_history);
  }

  const auto sort = AdsHistorySortFactory::Build(sort_type);
  if (sort) {
    ads_history = sort->Apply(ads_history);
  }

  AdsHistoryInfo normalized_ads_history;
  for (const auto& item : ads_history) {
    normalized_ads_history.items.push_back(item);
  }

  return normalized_ads_history;
}

void AddAdNotification(const AdNotificationInfo& ad,
                       const ConfirmationType& confirmation_type) {
  const AdHistoryInfo ad_history =
      BuildAdHistory(ad, confirmation_type, ad.title, ad.body);

  Client::Get()->AppendAdHistory(ad_history);
}

void AddNewTabPageAd(const NewTabPageAdInfo& ad,
                     const ConfirmationType& confirmation_type) {
  const AdHistoryInfo ad_history =
      BuildAdHistory(ad, confirmation_type, ad.company_name, ad.alt);

  Client::Get()->AppendAdHistory(ad_history);
}

void AddPromotedContentAd(const PromotedContentAdInfo& ad,
                          const ConfirmationType& confirmation_type) {
  const AdHistoryInfo ad_history =
      BuildAdHistory(ad, confirmation_type, ad.title, ad.description);

  Client::Get()->AppendAdHistory(ad_history);
}

void AddInlineContentAd(const InlineContentAdInfo& ad,
                        const ConfirmationType& confirmation_type) {
  const AdHistoryInfo ad_history =
      BuildAdHistory(ad, confirmation_type, ad.title, ad.description);

  Client::Get()->AppendAdHistory(ad_history);
}

}  // namespace history
}  // namespace ads
