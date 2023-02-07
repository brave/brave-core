/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions.h"

#include <set>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/common/url/url_util.h"
#include "bat/ads/internal/conversions/conversion_queue_database_table.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/conversions/conversions_features.h"
#include "bat/ads/internal/conversions/sorts/conversions_sort_factory.h"
#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_info.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_resource.h"
#include "bat/ads/internal/resources/country_components.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "brave_base/random.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace ads {

namespace {

constexpr int64_t kConvertAfterSeconds =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
constexpr int64_t kDebugConvertAfterSeconds =
    10 * base::Time::kSecondsPerMinute;
constexpr int64_t kExpiredConvertAfterSeconds =
    1 * base::Time::kSecondsPerMinute;
constexpr char kSearchInUrl[] = "url";

bool HasObservationWindowForAdEventExpired(const int observation_window,
                                           const AdEventInfo& ad_event) {
  const base::Time time = base::Time::Now() - base::Days(observation_window);

  return time >= ad_event.created_at;
}

bool ShouldConvertAdEvent(const AdEventInfo& ad_event) {
  if (ad_event.type == AdType::kInlineContentAd) {
    return ad_event.confirmation_type != ConfirmationType::kViewed;
  }

  // Do not convert if the user has not joined rewards for all other ad types
  return ShouldRewardUser();
}

bool DoesConfirmationTypeMatchConversionType(
    const ConfirmationType& confirmation_type,
    const std::string& conversion_type) {
  switch (confirmation_type.value()) {
    case ConfirmationType::kViewed: {
      return conversion_type == "postview";
    }

    case ConfirmationType::kClicked: {
      return conversion_type == "postclick";
    }

    case ConfirmationType::kUndefined:
    case ConfirmationType::kServed:
    case ConfirmationType::kDismissed:
    case ConfirmationType::kTransferred:
    case ConfirmationType::kSaved:
    case ConfirmationType::kFlagged:
    case ConfirmationType::kUpvoted:
    case ConfirmationType::kDownvoted:
    case ConfirmationType::kConversion: {
      return false;
    }
  }

  NOTREACHED() << "Unexpected value for ConfirmationType: "
               << static_cast<int>(confirmation_type.value());
  return false;
}

std::string ExtractConversionIdFromText(
    const std::string& html,
    const std::vector<GURL>& redirect_chain,
    const std::string& conversion_url_pattern,
    const ConversionIdPatternMap& conversion_id_patterns) {
  std::string conversion_id;
  std::string conversion_id_pattern = features::GetDefaultConversionIdPattern();
  std::string text = html;

  const auto iter = conversion_id_patterns.find(conversion_url_pattern);
  if (iter != conversion_id_patterns.cend()) {
    const ConversionIdPatternInfo conversion_id_pattern_info = iter->second;
    if (conversion_id_pattern_info.search_in == kSearchInUrl) {
      const auto url_iter = base::ranges::find_if(
          redirect_chain, [&conversion_url_pattern](const GURL& url) {
            return MatchUrlPattern(url, conversion_url_pattern);
          });

      if (url_iter == redirect_chain.cend()) {
        return conversion_id;
      }

      const GURL& url = *url_iter;
      text = url.spec();
    }

    conversion_id_pattern = conversion_id_pattern_info.id_pattern;
  }

  re2::StringPiece text_string_piece(text);
  const RE2 r(conversion_id_pattern);
  RE2::FindAndConsume(&text_string_piece, r, &conversion_id);

  return conversion_id;
}

std::set<std::string> GetConvertedCreativeSets(const AdEventList& ad_events) {
  std::set<std::string> creative_set_ids;
  for (const auto& ad_event : ad_events) {
    if (ad_event.confirmation_type != ConfirmationType::kConversion) {
      continue;
    }

    if (creative_set_ids.find(ad_event.creative_set_id) !=
        creative_set_ids.cend()) {
      continue;
    }

    creative_set_ids.insert(ad_event.creative_set_id);
  }

  return creative_set_ids;
}

AdEventList FilterAdEventsForConversion(const AdEventList& ad_events,
                                        const ConversionInfo& conversion) {
  AdEventList filtered_ad_events;

  base::ranges::copy_if(
      ad_events, std::back_inserter(filtered_ad_events),
      [&conversion](const AdEventInfo& ad_event) {
        if (ad_event.creative_set_id != conversion.creative_set_id) {
          return false;
        }

        if (!ShouldConvertAdEvent(ad_event)) {
          return false;
        }

        if (!DoesConfirmationTypeMatchConversionType(ad_event.confirmation_type,
                                                     conversion.type)) {
          return false;
        }

        if (HasObservationWindowForAdEventExpired(conversion.observation_window,
                                                  ad_event)) {
          return false;
        }

        return true;
      });

  return filtered_ad_events;
}

ConversionList FilterConversions(const std::vector<GURL>& redirect_chain,
                                 const ConversionList& conversions) {
  ConversionList filtered_conversions;

  base::ranges::copy_if(conversions, std::back_inserter(filtered_conversions),
                        [&redirect_chain](const ConversionInfo& conversion) {
                          const auto iter = base::ranges::find_if(
                              redirect_chain, [&conversion](const GURL& url) {
                                return MatchUrlPattern(url,
                                                       conversion.url_pattern);
                              });

                          return iter != redirect_chain.cend();
                        });

  return filtered_conversions;
}

ConversionList SortConversions(const ConversionList& conversions) {
  const auto sort =
      ConversionsSortFactory::Build(ConversionSortType::kDescendingOrder);
  DCHECK(sort);

  return sort->Apply(conversions);
}

}  // namespace

Conversions::Conversions() {
  resource_ = std::make_unique<resource::Conversions>();

  LocaleManager::GetInstance()->AddObserver(this);
  ResourceManager::GetInstance()->AddObserver(this);
  TabManager::GetInstance()->AddObserver(this);
}

Conversions::~Conversions() {
  LocaleManager::GetInstance()->RemoveObserver(this);
  ResourceManager::GetInstance()->RemoveObserver(this);
  TabManager::GetInstance()->RemoveObserver(this);
}

void Conversions::AddObserver(ConversionsObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Conversions::RemoveObserver(ConversionsObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Conversions::MaybeConvert(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const ConversionIdPatternMap& conversion_id_patterns) {
  if (redirect_chain.empty()) {
    return;
  }

  const GURL& url = redirect_chain.back();
  if (!SchemeIsSupported(url)) {
    BLOG(1, "URL is not supported for conversions");
    return;
  }

  CheckRedirectChain(redirect_chain, html, conversion_id_patterns);
}

void Conversions::Process() {
  const database::table::ConversionQueue database_table;
  database_table.GetUnprocessed(base::BindOnce(
      &Conversions::OnGetUnprocessedConversions, base::Unretained(this)));
}

///////////////////////////////////////////////////////////////////////////////

void Conversions::OnGetUnprocessedConversions(
    const bool success,
    const ConversionQueueItemList& conversion_queue_items) {
  if (!success) {
    BLOG(1, "Failed to get unprocessed conversions");
    return;
  }

  if (conversion_queue_items.empty()) {
    BLOG(1, "Conversion queue is empty");
    return;
  }

  const ConversionQueueItemInfo& conversion_queue_item =
      conversion_queue_items.front();

  StartTimer(conversion_queue_item);
}

void Conversions::CheckRedirectChain(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const ConversionIdPatternMap& conversion_id_patterns) {
  BLOG(1, "Checking URL for conversions");

  const database::table::AdEvents ad_events_database_table;
  ad_events_database_table.GetAll(
      base::BindOnce(&Conversions::OnGetAllAdEvents, base::Unretained(this),
                     redirect_chain, html, conversion_id_patterns));
}

void Conversions::OnGetAllAdEvents(
    std::vector<GURL> redirect_chain,
    std::string html,
    ConversionIdPatternMap conversion_id_patterns,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Failed to get ad events");
    return;
  }

  const database::table::Conversions conversions_database_table;
  conversions_database_table.GetAll(
      base::BindOnce(&Conversions::OnGetAllConversions, base::Unretained(this),
                     std::move(redirect_chain), std::move(html),
                     std::move(conversion_id_patterns), ad_events));
}

void Conversions::OnGetAllConversions(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const ConversionIdPatternMap& conversion_id_patterns,
    const AdEventList& ad_events,
    const bool success,
    const ConversionList& conversions) {
  if (!success) {
    BLOG(1, "Failed to get conversions");
    return;
  }

  if (conversions.empty()) {
    BLOG(1, "There are no conversions");
    return;
  }

  // Filter conversions by url pattern
  ConversionList filtered_conversions =
      FilterConversions(redirect_chain, conversions);

  // Sort conversions in descending order
  filtered_conversions = SortConversions(filtered_conversions);

  // Create list of creative set ids for already converted ads
  std::set<std::string> creative_set_ids = GetConvertedCreativeSets(ad_events);

  bool converted = false;

  // Check for conversions
  for (const auto& conversion : filtered_conversions) {
    const AdEventList filtered_ad_events =
        FilterAdEventsForConversion(ad_events, conversion);

    for (const auto& ad_event : filtered_ad_events) {
      if (creative_set_ids.find(conversion.creative_set_id) !=
          creative_set_ids.cend()) {
        // Creative set id has already been converted
        continue;
      }

      creative_set_ids.insert(ad_event.creative_set_id);

      VerifiableConversionInfo verifiable_conversion;
      verifiable_conversion.id = ExtractConversionIdFromText(
          html, redirect_chain, conversion.url_pattern, conversion_id_patterns);
      verifiable_conversion.public_key = conversion.advertiser_public_key;

      Convert(ad_event, verifiable_conversion);

      converted = true;
    }
  }

  if (!converted) {
    BLOG(1, "There were no conversion matches");
  } else {
    BLOG(1, "There was a conversion match");
  }
}

void Conversions::Convert(
    const AdEventInfo& ad_event,
    const VerifiableConversionInfo& verifiable_conversion) {
  BLOG(1, "Conversion for "
              << ad_event.type << " with campaign id " << ad_event.campaign_id
              << ", creative set id " << ad_event.creative_set_id
              << ", creative instance id " << ad_event.creative_instance_id
              << " and advertiser id " << ad_event.advertiser_id);

  AddItemToQueue(ad_event, verifiable_conversion);
}

void Conversions::AddItemToQueue(
    const AdEventInfo& ad_event,
    const VerifiableConversionInfo& verifiable_conversion) {
  AdEventInfo conversion_ad_event = ad_event;
  conversion_ad_event.created_at = base::Time::Now();
  conversion_ad_event.confirmation_type = ConfirmationType::kConversion;

  LogAdEvent(conversion_ad_event, base::BindOnce([](const bool success) {
               if (!success) {
                 BLOG(1, "Failed to log conversion event");
                 return;
               }

               BLOG(6, "Successfully logged conversion event");
             }));

  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.campaign_id = ad_event.campaign_id;
  conversion_queue_item.creative_set_id = ad_event.creative_set_id;
  conversion_queue_item.creative_instance_id = ad_event.creative_instance_id;
  conversion_queue_item.advertiser_id = ad_event.advertiser_id;
  conversion_queue_item.conversion_id = verifiable_conversion.id;
  conversion_queue_item.advertiser_public_key =
      verifiable_conversion.public_key;
  conversion_queue_item.ad_type = ad_event.type;
  const auto rand_delay = static_cast<int64_t>(brave_base::random::Geometric(
      FlagManager::GetInstance()->ShouldDebug() ? kDebugConvertAfterSeconds
                                                : kConvertAfterSeconds));
  conversion_queue_item.process_at =
      base::Time::Now() + base::Seconds(rand_delay);

  database::table::ConversionQueue database_table;
  database_table.Save({conversion_queue_item},
                      base::BindOnce(&Conversions::OnSaveConversionQueue,
                                     base::Unretained(this)));
}

void Conversions::OnSaveConversionQueue(const bool success) {
  if (!success) {
    BLOG(1, "Failed to append conversion to queue");
    return;
  }

  BLOG(3, "Successfully appended conversion to queue");

  Process();
}

void Conversions::ProcessQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  if (!conversion_queue_item.IsValid()) {
    RemoveInvalidQueueItem(conversion_queue_item);
    return;
  }

  MarkQueueItemAsProcessed(conversion_queue_item);
}

void Conversions::FailedToConvertQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  BLOG(1, "Failed to convert "
              << conversion_queue_item.ad_type << " with campaign id "
              << conversion_queue_item.campaign_id << ", creative set id "
              << conversion_queue_item.creative_set_id
              << ", creative instance id "
              << conversion_queue_item.creative_instance_id
              << " and advertiser id " << conversion_queue_item.advertiser_id
              << " "
              << LongFriendlyDateAndTime(conversion_queue_item.process_at,
                                         /*use_sentence_style*/ true));

  NotifyConversionFailed(conversion_queue_item);

  Process();
}

void Conversions::ConvertedQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  BLOG(1, "Successfully converted "
              << conversion_queue_item.ad_type << " with campaign id "
              << conversion_queue_item.campaign_id << ", creative set id "
              << conversion_queue_item.creative_set_id
              << ", creative instance id "
              << conversion_queue_item.creative_instance_id
              << " and advertiser id " << conversion_queue_item.advertiser_id
              << " "
              << LongFriendlyDateAndTime(conversion_queue_item.process_at,
                                         /*use_sentence_style*/ true));

  NotifyConversion(conversion_queue_item);

  Process();
}

void Conversions::ProcessQueue() {
  const database::table::ConversionQueue database_table;
  database_table.GetUnprocessed(base::BindOnce(
      &Conversions::OnGetConversionQueue, base::Unretained(this)));
}

void Conversions::OnGetConversionQueue(
    const bool success,
    const ConversionQueueItemList& conversion_queue_items) {
  if (!success) {
    BLOG(1, "Failed to get conversion queue");
    return;
  }

  if (conversion_queue_items.empty()) {
    BLOG(1, "Conversion queue is empty");
    return;
  }

  const ConversionQueueItemInfo& conversion_queue_item =
      conversion_queue_items.front();

  ProcessQueueItem(conversion_queue_item);
}

void Conversions::RemoveInvalidQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  const database::table::ConversionQueue database_table;
  database_table.Delete(
      conversion_queue_item,
      base::BindOnce(&Conversions::OnRemoveInvalidQueueItem,
                     base::Unretained(this), conversion_queue_item));
}

void Conversions::OnRemoveInvalidQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item,
    const bool success) {
  if (!success) {
    BLOG(0, "Failed to remove invalid conversion");
    NOTREACHED();
    return;
  }

  FailedToConvertQueueItem(conversion_queue_item);
}

void Conversions::MarkQueueItemAsProcessed(
    const ConversionQueueItemInfo& conversion_queue_item) {
  const database::table::ConversionQueue database_table;
  database_table.Update(
      conversion_queue_item,
      base::BindOnce(&Conversions::OnMarkQueueItemAsProcessed,
                     base::Unretained(this), conversion_queue_item));
}

void Conversions::OnMarkQueueItemAsProcessed(
    const ConversionQueueItemInfo& conversion_queue_item,
    const bool success) {
  if (!success) {
    BLOG(0, "Failed to mark conversion as processed");
    NOTREACHED();
    return;
  }

  ConvertedQueueItem(conversion_queue_item);
}

void Conversions::StartTimer(
    const ConversionQueueItemInfo& conversion_queue_item) {
  const base::Time now = base::Time::Now();

  base::TimeDelta delay;

  if (now < conversion_queue_item.process_at) {
    delay = conversion_queue_item.process_at - now;
  } else {
    const auto rand_delay = static_cast<int64_t>(
        brave_base::random::Geometric(kExpiredConvertAfterSeconds));
    delay = base::Seconds(rand_delay);
  }

  const base::Time process_queue_at = timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Conversions::ProcessQueue, base::Unretained(this)));

  BLOG(1, "Convert " << conversion_queue_item.ad_type << " with campaign id "
                     << conversion_queue_item.campaign_id
                     << ", creative set id "
                     << conversion_queue_item.creative_set_id
                     << ", creative instance id "
                     << conversion_queue_item.creative_instance_id
                     << " and advertiser id "
                     << conversion_queue_item.advertiser_id << " "
                     << FriendlyDateAndTime(process_queue_at,
                                            /*use_sentence_style*/ true));
}

void Conversions::NotifyConversion(
    const ConversionQueueItemInfo& conversion_queue_item) const {
  for (ConversionsObserver& observer : observers_) {
    observer.OnConversion(conversion_queue_item);
  }
}

void Conversions::NotifyConversionFailed(
    const ConversionQueueItemInfo& conversion_queue_item) const {
  for (ConversionsObserver& observer : observers_) {
    observer.OnConversionFailed(conversion_queue_item);
  }
}

void Conversions::OnLocaleDidChange(const std::string& /*locale*/) {
  resource_->Load();
}

void Conversions::OnResourceDidUpdate(const std::string& id) {
  if (IsValidCountryComponentId(id)) {
    resource_->Load();
  }
}

void Conversions::OnHtmlContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& content) {
  MaybeConvert(redirect_chain, content, resource_->get()->id_patterns);
}

}  // namespace ads
