/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history.h"

#include <deque>
#include <memory>

#include "base/time/time.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads_history/filters/ads_history_date_range_filter.h"
#include "bat/ads/internal/ads_history/filters/ads_history_filter_factory.h"
#include "bat/ads/internal/ads_history/sorts/ads_history_sort_factory.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/url_util.h"
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
  AdHistoryInfo ad_history;

  ad_history.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  ad_history.ad_content.type = ad.type;
  ad_history.ad_content.uuid = ad.uuid;
  ad_history.ad_content.creative_instance_id = ad.creative_instance_id;
  ad_history.ad_content.creative_set_id = ad.creative_set_id;
  ad_history.ad_content.campaign_id = ad.campaign_id;
  ad_history.ad_content.brand = ad.title;
  ad_history.ad_content.brand_info = ad.body;
  ad_history.ad_content.brand_display_url = GetHostFromUrl(ad.target_url);
  ad_history.ad_content.brand_url = ad.target_url;
  ad_history.ad_content.ad_action = confirmation_type;
  ad_history.category_content.category = ad.segment;

  Client::Get()->AppendAdHistoryToAdsHistory(ad_history);
}

void AddNewTabPageAd(const NewTabPageAdInfo& ad,
                     const ConfirmationType& confirmation_type) {
  AdHistoryInfo ad_history;

  ad_history.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  ad_history.ad_content.type = ad.type;
  ad_history.ad_content.uuid = ad.uuid;
  ad_history.ad_content.creative_instance_id = ad.creative_instance_id;
  ad_history.ad_content.creative_set_id = ad.creative_set_id;
  ad_history.ad_content.campaign_id = ad.campaign_id;
  ad_history.ad_content.brand = ad.company_name;
  ad_history.ad_content.brand_info = ad.alt;
  ad_history.ad_content.brand_display_url = GetHostFromUrl(ad.target_url);
  ad_history.ad_content.brand_url = ad.target_url;
  ad_history.ad_content.ad_action = confirmation_type;
  ad_history.category_content.category = ad.segment;

  Client::Get()->AppendAdHistoryToAdsHistory(ad_history);
}

void AddPromotedContentAd(const PromotedContentAdInfo& ad,
                          const ConfirmationType& confirmation_type) {
  AdHistoryInfo ad_history;

  ad_history.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  ad_history.ad_content.type = ad.type;
  ad_history.ad_content.uuid = ad.uuid;
  ad_history.ad_content.creative_instance_id = ad.creative_instance_id;
  ad_history.ad_content.creative_set_id = ad.creative_set_id;
  ad_history.ad_content.campaign_id = ad.campaign_id;
  ad_history.ad_content.brand = ad.title;
  ad_history.ad_content.brand_info = ad.description;
  ad_history.ad_content.brand_display_url = GetHostFromUrl(ad.target_url);
  ad_history.ad_content.brand_url = ad.target_url;
  ad_history.ad_content.ad_action = confirmation_type;
  ad_history.category_content.category = ad.segment;

  Client::Get()->AppendAdHistoryToAdsHistory(ad_history);
}

void AddInlineContentAd(const InlineContentAdInfo& ad,
                        const ConfirmationType& confirmation_type) {
  AdHistoryInfo ad_history;

  ad_history.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());
  ad_history.ad_content.type = ad.type;
  ad_history.ad_content.uuid = ad.uuid;
  ad_history.ad_content.creative_instance_id = ad.creative_instance_id;
  ad_history.ad_content.creative_set_id = ad.creative_set_id;
  ad_history.ad_content.campaign_id = ad.campaign_id;
  ad_history.ad_content.brand = ad.title;
  ad_history.ad_content.brand_info = ad.description;
  ad_history.ad_content.brand_display_url = GetHostFromUrl(ad.target_url);
  ad_history.ad_content.brand_url = ad.target_url;
  ad_history.ad_content.ad_action = confirmation_type;
  ad_history.category_content.category = ad.segment;

  Client::Get()->AppendAdHistoryToAdsHistory(ad_history);
}

}  // namespace history
}  // namespace ads
