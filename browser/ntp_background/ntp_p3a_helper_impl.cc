/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"

#include <vector>

#include "base/bind.h"
#include "base/check.h"
#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ntp_background_images {

namespace {

constexpr char kNewTabPageEventCountDictPref[] =
    "brave.brave_ads.p3a.ntp_event_count";

constexpr int kCountBuckets[] = {0, 1, 2, 3, 8, 12, 16};

constexpr char kViewEventKey[] = "views";
constexpr char kClickEventKey[] = "clicks";
constexpr char kLandEventKey[] = "lands";

constexpr char kInflightDictKey[] = "inflight";
constexpr char kExpireTimeKey[] = "expiry_time";

constexpr base::TimeDelta kCountExpiryTime = base::Days(30);

constexpr base::TimeDelta kStartLandingCheckTime = base::Milliseconds(750);
constexpr base::TimeDelta kLandingTime = base::Seconds(10);

}  // namespace

NTPP3AHelperImpl::NTPP3AHelperImpl(PrefService* local_state,
                                   brave::BraveP3AService* p3a_service)
    : local_state_(local_state), p3a_service_(p3a_service) {
  DCHECK(local_state);
  DCHECK(p3a_service);
  metric_sent_subscription_ =
      p3a_service->RegisterMetricSentCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3AMetricSent, base::Unretained(this)));
  rotation_subscription_ =
      p3a_service->RegisterRotationCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3ARotation, base::Unretained(this)));
}

NTPP3AHelperImpl::~NTPP3AHelperImpl() = default;

void NTPP3AHelperImpl::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kNewTabPageEventCountDictPref);
}

void NTPP3AHelperImpl::RecordView(const std::string& creative_instance_id) {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }
  UpdateMetricCount(creative_instance_id, kViewEventKey);
}

void NTPP3AHelperImpl::RecordClickAndMaybeLand(
    const std::string& creative_instance_id) {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }
  UpdateMetricCount(creative_instance_id, kClickEventKey);
  landing_check_timer_.Start(
      FROM_HERE, kStartLandingCheckTime,
      base::BindOnce(&NTPP3AHelperImpl::OnLandingStartCheck,
                     base::Unretained(this), creative_instance_id));
}

void NTPP3AHelperImpl::SetLastTabURL(const GURL& url) {
  last_tab_hostname_ = url.host();
}

void NTPP3AHelperImpl::OnP3ARotation(bool is_express) {
  if (!is_express) {
    return;
  }
  DictionaryPrefUpdate update(local_state_, kNewTabPageEventCountDictPref);
  base::Value::Dict& update_dict = update->GetDict();

  if (!p3a_service_->IsP3AEnabled()) {
    update_dict.clear();
    return;
  }

  for (const auto [creative_instance_id, creative_value] : update_dict) {
    base::Value::Dict& creative_dict = creative_value.GetDict();
    base::Value::Dict* inflight_dict = creative_dict.FindDict(kInflightDictKey);
    DCHECK(inflight_dict);
    for (const auto [key, value] : creative_dict) {
      if (key == kExpireTimeKey || key == kInflightDictKey) {
        continue;
      }
      int count = value.GetInt();
      inflight_dict->Set(key, count);
      p3a_utils::RecordToHistogramBucket(
          BuildHistogramName(creative_instance_id, key).c_str(), kCountBuckets,
          count);
    }
  }
}

void NTPP3AHelperImpl::OnP3AMetricSent(const std::string& histogram_name) {
  if (!base::StartsWith(histogram_name, brave::kCreativeMetricPrefix)) {
    return;
  }

  std::vector<std::string> histogram_name_tokens = base::SplitString(
      histogram_name, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);
  if (histogram_name_tokens.size() != 3) {
    return;
  }

  const std::string& creative_instance_id = histogram_name_tokens[1];
  const std::string& event_type = histogram_name_tokens[2];

  DictionaryPrefUpdate update(local_state_, kNewTabPageEventCountDictPref);
  base::Value::Dict& update_dict = update->GetDict();

  base::Value::Dict* creative_dict = update_dict.FindDict(creative_instance_id);
  if (creative_dict == nullptr) {
    p3a_service_->RemoveDynamicMetric(histogram_name);
    return;
  }
  base::Value::Dict* inflight_dict = creative_dict->FindDict(kInflightDictKey);
  DCHECK(inflight_dict);
  if (inflight_dict == nullptr) {
    // Unexpected condition, return to avoid crash.
    return;
  }

  int full_count = creative_dict->FindInt(event_type).value_or(0);
  int inflight_count = inflight_dict->FindInt(event_type).value_or(0);
  int new_count = full_count - inflight_count;

  inflight_dict->Remove(event_type);

  if (new_count > 0) {
    creative_dict->Set(event_type, new_count);
  } else {
    p3a_service_->RemoveDynamicMetric(histogram_name);
    creative_dict->Remove(event_type);
    // If the only elements left in the dict are expiry time and inflight dict,
    // then remove the creative dict
    if (creative_dict->size() <= 2) {
      update_dict.Remove(creative_instance_id);
    }
  }
}

std::string NTPP3AHelperImpl::BuildHistogramName(
    const std::string& creative_instance_id,
    const std::string& event_type) {
  return base::StrCat(
      {brave::kCreativeMetricPrefix, creative_instance_id, ".", event_type});
}

void NTPP3AHelperImpl::UpdateMetricCount(
    const std::string& creative_instance_id,
    const std::string& event_type) {
  const std::string histogram_name =
      BuildHistogramName(creative_instance_id, event_type);

  p3a_service_->RegisterDynamicMetric(histogram_name,
                                      brave::MetricLogType::kExpress);

  DictionaryPrefUpdate update(local_state_, kNewTabPageEventCountDictPref);

  base::Value::Dict& update_dict = update->GetDict();

  base::Value* creative_instance_value = update_dict.Find(creative_instance_id);
  if (creative_instance_value == nullptr ||
      !creative_instance_value->is_dict()) {
    creative_instance_value =
        update_dict.Set(creative_instance_id, base::Value::Dict());
    creative_instance_value->GetDict().Set(kInflightDictKey,
                                           base::Value::Dict());
  }
  base::Value::Dict& creative_instance_dict =
      creative_instance_value->GetDict();

  const absl::optional<int> current_value =
      creative_instance_dict.FindInt(event_type);

  const int count = current_value.value_or(0) + 1;

  creative_instance_dict.Set(event_type, count);
  const base::Time new_expiry_time = base::Time::Now() + kCountExpiryTime;
  creative_instance_dict.Set(kExpireTimeKey,
                             base::TimeToValue(new_expiry_time).GetString());
}

void NTPP3AHelperImpl::OnLandingStartCheck(
    const std::string& creative_instance_id) {
  if (!last_tab_hostname_.has_value()) {
    return;
  }
  landing_check_timer_.Start(
      FROM_HERE, kLandingTime,
      base::BindOnce(&NTPP3AHelperImpl::OnLandingEndCheck,
                     base::Unretained(this), creative_instance_id,
                     *last_tab_hostname_));
}

void NTPP3AHelperImpl::OnLandingEndCheck(
    const std::string& creative_instance_id,
    const std::string& expected_hostname) {
  if (!last_tab_hostname_.has_value() ||
      last_tab_hostname_ != expected_hostname) {
    return;
  }
  UpdateMetricCount(creative_instance_id, kLandEventKey);
}

}  // namespace ntp_background_images
