/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions.h"

#include <stdint.h>

#include <algorithm>
#include <functional>
#include <set>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave_base/random.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/conversions/sorts/conversions_sort_factory.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/conversions_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/internal/url_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

const char kConversionsFilename[] = "ad_conversions.json";

const char kConversionsListKey[] = "ad_conversions";
const char kConversionTimestampKey[] = "timestamp_in_seconds";
const char kConversionCreativeSetIdKey[] = "creative_set_id";
const char kConversionCreativeInstanceIdKey[] = "uuid";

const int64_t kConvertAfterSeconds =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;
const int64_t kDebugConvertAfterSeconds = 10 * base::Time::kSecondsPerMinute;
const int64_t kExpiredConvertAfterSeconds = 1 * base::Time::kSecondsPerMinute;

bool HasObservationWindowForAdEventExpired(
    const int observation_window,
    const AdEventInfo& ad_event) {
  const base::Time observation_window_time = base::Time::Now() -
      base::TimeDelta::FromDays(observation_window);

  const base::Time time = base::Time::FromDoubleT(ad_event.timestamp);

  if (observation_window_time < time) {
    return false;
  }

  return true;
}

}  // namespace

Conversions::Conversions() = default;

Conversions::~Conversions() = default;

void Conversions::AddObserver(
    ConversionsObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Conversions::RemoveObserver(
    ConversionsObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Conversions::Initialize(
    InitializeCallback callback) {
  callback_ = callback;

  Load();
}

void Conversions::MaybeConvert(
    const std::vector<std::string>& redirect_chain) {
  DCHECK(is_initialized_);

  if (!ShouldAllow()) {
    BLOG(1, "Conversions are not allowed");
    return;
  }

  const std::string url = redirect_chain.back();
  if (!DoesUrlHaveSchemeHTTPOrHTTPS(url)) {
    BLOG(1, "URL is not supported for conversions");
    return;
  }

  CheckRedirectChain(redirect_chain);
}

void Conversions::StartTimerIfReady() {
  DCHECK(is_initialized_);

  if (timer_.IsRunning()) {
    return;
  }

  if (queue_.empty()) {
    BLOG(1, "Conversions queue is empty");
    return;
  }

  ConversionQueueItemInfo queue_item = queue_.front();
  StartTimer(queue_item);
}

///////////////////////////////////////////////////////////////////////////////

bool Conversions::ShouldAllow() const {
  return AdsClientHelper::Get()->GetBooleanPref(
      prefs::kShouldAllowConversionTracking);
}

void Conversions::CheckRedirectChain(
    const std::vector<std::string>& redirect_chain) {
  BLOG(1, "Checking URL for conversions");

  database::table::AdEvents ad_events_database_table;
  ad_events_database_table.GetAll([=](
      const Result result,
      const AdEventList& ad_events) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to get ad events");
      return;
    }

    database::table::Conversions conversions_database_table;
    conversions_database_table.GetAll([=](
        const Result result,
        const ConversionList& conversions) {
      if (result != SUCCESS) {
        BLOG(1, "Failed to get conversions");
        return;
      }

      if (conversions.empty()) {
        BLOG(1, "No conversions found for visited URL");
        return;
      }

      // Filter conversions by url pattern
      ConversionList filtered_conversions =
          FilterConversions(redirect_chain, conversions);

      // Sort conversions in descending order
      filtered_conversions = SortConversions(filtered_conversions);

      // Create list of creative set ids for already converted ads
      std::set<std::string> creative_set_ids;
      for (const auto& ad_event : ad_events) {
        if (ad_event.confirmation_type != ConfirmationType::kConversion) {
          continue;
        }

        if (creative_set_ids.find(ad_event.creative_set_id) !=
            creative_set_ids.end()) {
          continue;
        }

        creative_set_ids.insert(ad_event.creative_set_id);
      }

      bool converted = false;

      // Check if ad events match conversions for views/clicks, expire timestamp
      // and creative set id
      for (const auto& conversion : filtered_conversions) {
        AdEventList filtered_ad_events = ad_events;
        const auto iter = std::remove_if(filtered_ad_events.begin(),
            filtered_ad_events.end(), [&conversion](
                const AdEventInfo& ad_event) {
          if (ad_event.creative_set_id != conversion.creative_set_id) {
            return true;
          }

          if (ad_event.confirmation_type != ConfirmationType::kViewed &&
              ad_event.confirmation_type != ConfirmationType::kClicked) {
            return true;
          }

          if (HasObservationWindowForAdEventExpired(
              conversion.observation_window, ad_event)) {
            return true;
          }

          return false;
        });
        filtered_ad_events.erase(iter, filtered_ad_events.end());

        // Check if already converted
        for (const auto& ad_event : filtered_ad_events) {
          if (creative_set_ids.find(conversion.creative_set_id) !=
              creative_set_ids.end()) {
            // Creative set id has already been converted
            continue;
          }

          creative_set_ids.insert(ad_event.creative_set_id);

          Convert(ad_event);

          converted = true;
        }
      }

      if (!converted) {
        BLOG(1, "No conversions found for visited URL");
      }
    });
  });
}

void Conversions::Convert(
    const AdEventInfo& ad_event) {
  BLOG(1, "Conversion for creative set id " << ad_event.creative_set_id
      << " and " << std::string(ad_event.type));

  AddItemToQueue(ad_event);
}

ConversionList Conversions::FilterConversions(
    const std::vector<std::string>& redirect_chain,
    const ConversionList& conversions) {
  ConversionList filtered_conversions = conversions;

  const auto iter = std::remove_if(filtered_conversions.begin(),
      filtered_conversions.end(), [&redirect_chain](
          const ConversionInfo& conversion) {
    const auto iter = std::find_if(redirect_chain.begin(), redirect_chain.end(),
        [&conversion](const std::string& url) {
      return DoesUrlMatchPattern(url, conversion.url_pattern);
    });

    if (iter != redirect_chain.end()) {
      return false;
    }

    return true;
  });

  filtered_conversions.erase(iter, filtered_conversions.end());

  return filtered_conversions;
}

ConversionList Conversions::SortConversions(
    const ConversionList& conversions) {
  const auto sort = ConversionsSortFactory::Build(
      ConversionInfo::SortType::kDescendingOrder);
  DCHECK(sort);

  return sort->Apply(conversions);
}

void Conversions::AddItemToQueue(
    const AdEventInfo& ad_event) {
  DCHECK(is_initialized_);

  AdEventInfo conversion_ad_event;
  conversion_ad_event.type = ad_event.type;
  conversion_ad_event.uuid = ad_event.uuid;
  conversion_ad_event.creative_instance_id = ad_event.creative_instance_id;
  conversion_ad_event.creative_set_id = ad_event.creative_set_id;
  conversion_ad_event.campaign_id = ad_event.campaign_id;
  conversion_ad_event.timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());
  conversion_ad_event.confirmation_type = ConfirmationType::kConversion;

  LogAdEvent(conversion_ad_event, [](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log conversion event");
      return;
    }

    BLOG(6, "Successfully logged conversion event");
  });

  ConversionQueueItemInfo queue_item;

  const uint64_t rand_delay = brave_base::random::Geometric(
      g_is_debug ? kDebugConvertAfterSeconds : kConvertAfterSeconds);

  const uint64_t now = static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  queue_item.timestamp_in_seconds = now + rand_delay;
  queue_item.creative_instance_id = ad_event.creative_instance_id;
  queue_item.creative_set_id = ad_event.creative_set_id;

  queue_.push_back(queue_item);

  std::sort(queue_.begin(), queue_.end(), [](
      const ConversionQueueItemInfo& a,
      const ConversionQueueItemInfo& b) {
    return a.timestamp_in_seconds < b.timestamp_in_seconds;
  });

  Save();

  StartTimerIfReady();
}

bool Conversions::RemoveItemFromQueue(
    const std::string& creative_instance_id) {
  DCHECK(is_initialized_);

  auto iter = std::find_if(queue_.begin(), queue_.end(),
      [&creative_instance_id](const ConversionQueueItemInfo& queue_item) {
    return queue_item.creative_instance_id == creative_instance_id;
  });

  if (iter == queue_.end()) {
    return false;
  }

  queue_.erase(iter);

  Save();

  return true;
}

void Conversions::ProcessQueueItem(
    const ConversionQueueItemInfo& queue_item) {
  const std::string friendly_date_and_time =
      FriendlyDateAndTime(queue_item.timestamp_in_seconds);

  const std::string creative_set_id = queue_item.creative_set_id;
  const std::string creative_instance_id = queue_item.creative_instance_id;

  if (!queue_item.IsValid()) {
    BLOG(1, "Failed to convert ad with creative instance id "
        << creative_instance_id << " and creative set id " << creative_set_id
            << " " << friendly_date_and_time);

    NotifyConversionFailed(creative_instance_id);
  } else {
    BLOG(1, "Successfully converted ad with creative instance id "
        << creative_instance_id << " and creative set id " << creative_set_id
            << " " << friendly_date_and_time);

    NotifyConversion(creative_instance_id);
  }

  RemoveItemFromQueue(queue_item.creative_instance_id);

  StartTimerIfReady();
}

void Conversions::ProcessQueue() {
  if (queue_.empty()) {
    return;
  }

  ConversionQueueItemInfo queue_item = queue_.front();
  ProcessQueueItem(queue_item);
}

void Conversions::StartTimer(
    const ConversionQueueItemInfo& queue_item) {
  DCHECK(is_initialized_);
  DCHECK(!timer_.IsRunning());

  const base::Time now = base::Time::Now();
  const base::Time timestamp =
      base::Time::FromDoubleT(queue_item.timestamp_in_seconds);

  base::TimeDelta delay;
  if (now < timestamp) {
    delay = timestamp - now;
  } else {
    const uint64_t rand_delay =
        brave_base::random::Geometric(kExpiredConvertAfterSeconds);
    delay = base::TimeDelta::FromSeconds(rand_delay);
  }

  const base::Time time = timer_.Start(delay,
      base::BindOnce(&Conversions::ProcessQueue, base::Unretained(this)));

  BLOG(1, "Convert creative instance id " << queue_item.creative_instance_id
      << " and creative set id " << queue_item.creative_set_id << " "
          << FriendlyDateAndTime(time));
}

void Conversions::Save() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving conversions state");

  std::string json = ToJson();
  auto callback = std::bind(&Conversions::OnSaved, this, std::placeholders::_1);
  AdsClientHelper::Get()->Save(kConversionsFilename, json, callback);
}

void Conversions::OnSaved(
    const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to save conversions state");
    return;
  }

  BLOG(9, "Successfully saved conversions state");
}

std::string Conversions::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto conversions = GetAsList();
  dictionary.SetKey(kConversionsListKey, base::Value(std::move(conversions)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

base::Value Conversions::GetAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& item : queue_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey(kConversionTimestampKey,
        base::Value(std::to_string(item.timestamp_in_seconds)));
    dictionary.SetKey(kConversionCreativeInstanceIdKey,
        base::Value(item.creative_instance_id));
    dictionary.SetKey(kConversionCreativeSetIdKey,
        base::Value(item.creative_set_id));

    list.Append(std::move(dictionary));
  }

  return list;
}

void Conversions::Load() {
  BLOG(3, "Loading conversions state");

  auto callback = std::bind(&Conversions::OnLoaded, this,
      std::placeholders::_1, std::placeholders::_2);
  AdsClientHelper::Get()->Load(kConversionsFilename, callback);
}

void Conversions::OnLoaded(
    const Result result,
    const std::string& json) {
  if (result != SUCCESS) {
    BLOG(3, "Conversions state does not exist, creating default state");

    is_initialized_ = true;

    queue_.clear();
    Save();
  } else {
    if (!FromJson(json)) {
      BLOG(0, "Failed to load conversions state");

      BLOG(3, "Failed to parse conversions state: " << json);

      callback_(FAILED);
      return;
    }

    BLOG(3, "Successfully loaded conversions state");

    is_initialized_ = true;
  }

  callback_(SUCCESS);
}

bool Conversions::FromJson(
    const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  auto* conversions_list_value = dictionary->FindKey(kConversionsListKey);
  if (!conversions_list_value || !conversions_list_value->is_list()) {
    return false;
  }

  base::ListValue* list = nullptr;
  if (!conversions_list_value->GetAsList(&list)) {
    return false;
  }

  queue_ = GetFromList(list);

  Save();

  return true;
}

ConversionQueueItemList Conversions::GetFromList(
    const base::ListValue* list) const {
  ConversionQueueItemList queue_items;

  DCHECK(list);
  if (!list) {
    return queue_items;
  }

  for (const auto& value : list->GetList()) {
    if (!value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* dictionary = nullptr;
    value.GetAsDictionary(&dictionary);
    if (!dictionary) {
      NOTREACHED();
      continue;
    }

    ConversionQueueItemInfo queue_item;
    if (!GetFromDictionary(dictionary, &queue_item)) {
      NOTREACHED();
      continue;
    }

    queue_items.push_back(queue_item);
  }

  return queue_items;
}

bool Conversions::GetFromDictionary(
    const base::DictionaryValue* dictionary,
    ConversionQueueItemInfo* info) const {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  DCHECK(info);
  if (!info) {
    return false;
  }

  ConversionQueueItemInfo queue_item;

  const auto* timestamp = dictionary->FindStringKey(kConversionTimestampKey);
  if (!timestamp) {
    return false;
  }
  if (!base::StringToUint64(*timestamp, &queue_item.timestamp_in_seconds)) {
    return false;
  }

  // Creative Set Id
  const auto* creative_set_id =
      dictionary->FindStringKey(kConversionCreativeSetIdKey);
  if (!creative_set_id) {
    return false;
  }
  queue_item.creative_set_id = *creative_set_id;

  // UUID
  const auto* creative_instance_id =
      dictionary->FindStringKey(kConversionCreativeInstanceIdKey);
  if (!creative_instance_id) {
    return false;
  }
  queue_item.creative_instance_id = *creative_instance_id;

  *info = queue_item;

  return true;
}

void Conversions::NotifyConversion(
    const std::string& creative_instance_id) {
  for (ConversionsObserver& observer : observers_) {
    observer.OnConversion(creative_instance_id);
  }
}

void Conversions::NotifyConversionFailed(
    const std::string& creative_instance_id) {
  for (ConversionsObserver& observer : observers_) {
    observer.OnConversionFailed(creative_instance_id);
  }
}

}  // namespace ads
