/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <deque>
#include <memory>
#include <utility>

#include "bat/ads/internal/ad_conversions.h"
#include "bat/ads/internal/filters/ads_history_filter_factory.h"
#include "bat/ads/internal/sorts/ad_conversions_sort_factory.h"
#include "bat/ads/internal/sorts/ads_history_sort_factory.h"
#include "bat/ads/internal/uri_helper.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time.h"
#include "brave_base/random.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ads {

const char kAdConversionsStateName[] = "ad_conversions.json";

const char kAdConversionsListKey[] = "ad_conversions";

const char kAdConversionTimestampKey[] = "timestamp_in_seconds";
const char kAdConversionCreativeSetIdKey[] = "creative_set_id";
const char kAdConversionCreativeInstanceeIdKey[] = "uuid";

AdConversions::AdConversions(
    AdsImpl* ads,
    AdsClient* ads_client,
    Client* client)
    : is_initialized_(false),
      timer_id_(0),
      ads_(ads),
      ads_client_(ads_client),
      client_(client) {
  DCHECK(ads_);
  DCHECK(ads_client_);
  DCHECK(client_);
}

AdConversions::~AdConversions() {
  StopTimer();
}

void AdConversions::Initialize(
    InitializeCallback callback) {
  callback_ = callback;

  LoadState();
}

void AdConversions::Check(
    const std::string& url) {
  DCHECK(is_initialized_);
  DCHECK(!url.empty());

  if (!ads_client_->ShouldAllowAdConversionTracking()) {
    return;
  }

  BLOG(INFO) << "Checking ad conversions for " << url;

  auto callback =
      std::bind(&AdConversions::OnGetAdConversions, this, url, _1, _2);
  ads_client_->GetAdConversions(callback);
}

void AdConversions::StartTimerIfReady() {
  DCHECK(is_initialized_);

  if (timer_id_ != 0) {
    return;
  }

  if (queue_.empty()) {
    BLOG(INFO) << "Ad conversion queue is empty";
    return;
  }

  AdConversionQueueItemInfo ad_conversion = queue_.front();
  StartTimer(ad_conversion);
}

bool AdConversions::OnTimer(
    const uint32_t timer_id) {
  if (timer_id != timer_id_) {
    return false;
  }

  timer_id_ = 0;

  DCHECK(!queue_.empty());
  if (queue_.empty()) {
    return true;
  }

  AdConversionQueueItemInfo ad_conversion = queue_.front();
  ProcessQueueItem(ad_conversion);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void AdConversions::OnGetAdConversions(
    const std::string& url,
    const Result result,
    const AdConversionList& ad_conversions) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to check ad conversions";
    return;
  }

  std::deque<AdHistory> ads_history = client_->GetAdsShownHistory();
  ads_history = FilterAdsHistory(ads_history);
  ads_history = SortAdsHistory(ads_history);

  AdConversionList new_ad_conversions = ad_conversions;
  new_ad_conversions = FilterAdConversions(url, new_ad_conversions);
  new_ad_conversions = SortAdConversions(new_ad_conversions);

  for (const auto& ad_conversion : new_ad_conversions) {
    for (const auto& ad : ads_history) {
      auto ad_conversion_history = client_->GetAdConversionHistory();
      if (ad_conversion_history.find(ad_conversion.creative_set_id) !=
          ad_conversion_history.end()) {
        // Creative set id has already been converted
        continue;
      }

      if (ad_conversion.creative_set_id != ad.ad_content.creative_set_id) {
        // Creative set id does not match
        continue;
      }

      const base::Time observation_window = base::Time::Now() -
          base::TimeDelta::FromDays(ad_conversion.observation_window);
      const base::Time time = Time::FromDoubleT(ad.timestamp_in_seconds);
      if (observation_window > time) {
        // Observation window has expired
        continue;
      }

      BLOG(INFO) << "Ad conversion for " << url << " with "
          << ad_conversion.creative_set_id << " creative set id for "
              << std::string(ad_conversion.type);

      AddItemToQueue(ad.ad_content.creative_instance_id,
          ad.ad_content.creative_set_id);
    }
  }
}

std::deque<AdHistory> AdConversions::FilterAdsHistory(
    const std::deque<AdHistory>& ads_history) {
  const auto filter = AdsHistoryFilterFactory::Build(
      AdsHistory::FilterType::kAdConversionConfirmationType);
  DCHECK(filter);

  return filter->Apply(ads_history);
}

std::deque<AdHistory> AdConversions::SortAdsHistory(
    const std::deque<AdHistory>& ads_history) {
  const auto sort = AdsHistorySortFactory::Build(
      AdsHistory::SortType::kDescendingOrder);
  DCHECK(sort);

  return sort->Apply(ads_history);
}

AdConversionList AdConversions::FilterAdConversions(
    const std::string& url,
    const AdConversionList& ad_conversions) {
  AdConversionList new_ad_conversions = ad_conversions;
  const auto iter = std::remove_if(new_ad_conversions.begin(),
      new_ad_conversions.end(), [&](const AdConversionInfo& info) {
    return !helper::Uri::MatchesWildcard(url, info.url_pattern);
  });
  new_ad_conversions.erase(iter, new_ad_conversions.end());

  return new_ad_conversions;
}

AdConversionList AdConversions::SortAdConversions(
    const AdConversionList& ad_conversions) {
  const auto sort = AdConversionsSortFactory::Build(
      AdConversionInfo::SortType::kDescendingOrder);
  DCHECK(sort);

  return sort->Apply(ad_conversions);
}

void AdConversions::AddItemToQueue(
    const std::string& creative_instance_id,
    const std::string& creative_set_id) {
  DCHECK(is_initialized_);
  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  if (creative_instance_id.empty() || creative_set_id.empty()) {
    return;
  }

  const uint64_t now = Time::NowInSeconds();
  client_->AppendTimestampToAdConversionHistory(creative_set_id, now);

  AdConversionQueueItemInfo ad_conversion;

  const uint64_t rand_delay = brave_base::random::Geometric(
      _is_debug ? kDebugAdConversionFrequency : kAdConversionFrequency);

  ad_conversion.timestamp_in_seconds = now + rand_delay;
  ad_conversion.creative_instance_id = creative_instance_id;
  ad_conversion.creative_set_id = creative_set_id;

  queue_.push_back(ad_conversion);

  std::sort(queue_.begin(), queue_.end(), [](
      const AdConversionQueueItemInfo& a,
      const AdConversionQueueItemInfo& b) {
    return a.timestamp_in_seconds < b.timestamp_in_seconds;
  });

  SaveState();

  StartTimerIfReady();
}

bool AdConversions::RemoveItemFromQueue(
    const std::string& creative_instance_id) {
  DCHECK(is_initialized_);

  auto iter = std::find_if(queue_.begin(), queue_.end(),
      [&creative_instance_id] (const auto& ad_conversion) {
    return ad_conversion.creative_instance_id == creative_instance_id;
  });

  if (iter == queue_.end()) {
    return false;
  }

  queue_.erase(iter);

  SaveState();

  return true;
}

void AdConversions::ProcessQueueItem(
    const AdConversionQueueItemInfo& info) {
  const uint64_t timestamp_in_seconds = info.timestamp_in_seconds;
  const std::string creative_set_id = info.creative_set_id;
  const std::string creative_instance_id = info.creative_instance_id;

  DCHECK(!creative_set_id.empty());
  DCHECK(!creative_instance_id.empty());

  if (creative_set_id.empty() || creative_instance_id.empty()) {
    BLOG(WARNING) << "Ad conversion for creative instance id "
        << creative_instance_id << " with creative set id " << creative_set_id
            << " failed on " << Time::FromDoubleT(timestamp_in_seconds);
  } else {
    BLOG(INFO) << "Ad conversion for creative instance id "
        << creative_instance_id << " with creative set id " << creative_set_id
            << " triggered on " << Time::FromDoubleT(timestamp_in_seconds);

    ads_->ConfirmAction(creative_instance_id, creative_set_id,
        ConfirmationType::kConversion);
  }

  RemoveItemFromQueue(creative_instance_id);

  StartTimerIfReady();
}

void AdConversions::StartTimer(
    const AdConversionQueueItemInfo& info) {
  DCHECK(is_initialized_);
  DCHECK_EQ(0UL, timer_id_);

  StopTimer();

  const uint64_t now = Time::NowInSeconds();

  uint64_t start_timer_in;
  if (now < info.timestamp_in_seconds) {
    start_timer_in = info.timestamp_in_seconds - now;
  } else {
    start_timer_in = brave_base::random::Geometric(
        kExpiredAdConversionFrequency);
  }

  timer_id_ = ads_client_->SetTimer(start_timer_in);
  if (timer_id_ == 0) {
    BLOG(ERROR) << "Failed to start ad conversion timer";
    return;
  }

  BLOG(INFO) << "Started ad conversion timer for creative_instance_id "
      << info.creative_instance_id << " with creative set id "
          << info.creative_set_id << " which will trigger on "
              << Time::FromDoubleT(info.timestamp_in_seconds);
}

void AdConversions::StopTimer() {
  if (timer_id_ == 0) {
    return;
  }

  BLOG(INFO) << "Stopped ad conversion timer";

  ads_client_->KillTimer(timer_id_);
  timer_id_ = 0;
}

void AdConversions::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(INFO) << "Saving ad conversions state";

  std::string json = ToJson();
  auto callback = std::bind(&AdConversions::OnStateSaved, this, _1);
  ads_client_->Save(kAdConversionsStateName, json, callback);
}

void AdConversions::OnStateSaved(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save ad conversions state";
    return;
  }

  BLOG(INFO) << "Successfully saved ad conversions state";
}

std::string AdConversions::ToJson() {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto ad_conversions = GetAsList();
  dictionary.SetKey(kAdConversionsListKey,
      base::Value(std::move(ad_conversions)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

base::Value AdConversions::GetAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& ad_conversion : queue_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey(kAdConversionTimestampKey,
        base::Value(std::to_string(ad_conversion.timestamp_in_seconds)));
    dictionary.SetKey(kAdConversionCreativeInstanceeIdKey,
        base::Value(ad_conversion.creative_instance_id));
    dictionary.SetKey(kAdConversionCreativeSetIdKey,
        base::Value(ad_conversion.creative_set_id));

    list.Append(std::move(dictionary));
  }

  return list;
}

void AdConversions::LoadState() {
  auto callback = std::bind(&AdConversions::OnStateLoaded, this, _1, _2);
  ads_client_->Load(kAdConversionsStateName, callback);
}

void AdConversions::OnStateLoaded(
    const Result result,
    const std::string& json) {
  is_initialized_ = true;

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load ad conversions state, resetting to default "
        "values";

    queue_.clear();
    SaveState();
  } else {
    if (!FromJson(json)) {
      BLOG(ERROR) << "Failed to parse ad conversions state: " << json;

      callback_(FAILED);
      return;
    }

    BLOG(INFO) << "Successfully loaded ad conversions state";
  }

  callback_(SUCCESS);
}

bool AdConversions::FromJson(
    const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  auto* ad_conversions_list_value = dictionary->FindKey(kAdConversionsListKey);
  if (!ad_conversions_list_value || !ad_conversions_list_value->is_list()) {
    return false;
  }

  base::ListValue* list = nullptr;
  if (!ad_conversions_list_value->GetAsList(&list)) {
    return false;
  }

  queue_ = GetFromList(list);

  SaveState();

  return true;
}

AdConversionQueueItemList AdConversions::GetFromList(
    const base::ListValue* list) const {
  AdConversionQueueItemList ad_conversions;

  DCHECK(list);
  if (!list) {
    return ad_conversions;
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

    AdConversionQueueItemInfo ad_conversion;
    if (!GetFromDictionary(dictionary, &ad_conversion)) {
      NOTREACHED();
      continue;
    }

    ad_conversions.push_back(ad_conversion);
  }

  return ad_conversions;
}

bool AdConversions::GetFromDictionary(
    const base::DictionaryValue* dictionary,
    AdConversionQueueItemInfo* info) const {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  DCHECK(info);
  if (!info) {
    return false;
  }

  AdConversionQueueItemInfo ad_conversion;

  const auto* timestamp = dictionary->FindStringKey(kAdConversionTimestampKey);
  if (!timestamp) {
    return false;
  }
  ad_conversion.timestamp_in_seconds = std::stoull(*timestamp);

  // Creative Set Id
  const auto* creative_set_id =
      dictionary->FindStringKey(kAdConversionCreativeSetIdKey);
  if (!creative_set_id) {
    return false;
  }
  ad_conversion.creative_set_id = *creative_set_id;

  // UUID
  const auto* creative_instance_id =
      dictionary->FindStringKey(kAdConversionCreativeInstanceeIdKey);
  if (!creative_instance_id) {
    return false;
  }
  ad_conversion.creative_instance_id = *creative_instance_id;

  *info = ad_conversion;

  return true;
}

}  // namespace ads
