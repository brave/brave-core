/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ntp_background_images {

namespace {

constexpr char kNewTabPageEventCountDictPref[] =
    "brave.brave_ads.p3a.ntp_event_count";
constexpr char kNewTabPageEventCountConstellationDictPref[] =
    "brave.brave_ads.p3a.ntp_event_count_constellation";
const char* kAllCountDicts[] = {kNewTabPageEventCountDictPref,
                                kNewTabPageEventCountConstellationDictPref};

constexpr int kCountBuckets[] = {0, 1, 2, 3, 8, 12, 16};

constexpr char kViewEventKey[] = "views";
constexpr char kClickEventKey[] = "clicks";
constexpr char kLandEventKey[] = "lands";

constexpr char kCreativeTotalCountHistogramName[] =
    "creativeInstanceId.total.count";

constexpr char kInflightDictKey[] = "inflight";
constexpr char kExpireTimeKey[] = "expiry_time";

constexpr base::TimeDelta kCountExpiryTime = base::Days(30);

constexpr base::TimeDelta kStartLandingCheckTime = base::Milliseconds(750);

bool IsRewardsDisabled(PrefService* prefs) {
  return !prefs->GetBoolean(brave_rewards::prefs::kEnabled) &&
         !base::FeatureList::IsEnabled(
             brave_ads::kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);
}

const char* GetCountDictPref(bool is_constellation) {
  if (is_constellation) {
    return kNewTabPageEventCountConstellationDictPref;
  } else {
    return kNewTabPageEventCountDictPref;
  }
}
}  // namespace

NTPP3AHelperImpl::NTPP3AHelperImpl(PrefService* local_state,
                                   p3a::P3AService* p3a_service,
                                   PrefService* prefs,
                                   bool use_uma_for_testing)
    : local_state_(local_state),
      p3a_service_(p3a_service),
      prefs_(prefs),
      use_uma_for_testing_(use_uma_for_testing) {
  DCHECK(local_state);
  DCHECK(p3a_service);
  DCHECK(prefs);
  metric_sent_subscription_ =
      p3a_service->RegisterMetricCycledCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3AMetricCycled, base::Unretained(this)));
  rotation_subscription_ =
      p3a_service->RegisterRotationCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3ARotation, base::Unretained(this)));
}

NTPP3AHelperImpl::~NTPP3AHelperImpl() = default;

void NTPP3AHelperImpl::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kNewTabPageEventCountDictPref);
  registry->RegisterDictionaryPref(kNewTabPageEventCountConstellationDictPref);
}

void NTPP3AHelperImpl::RecordView(const std::string& creative_instance_id) {
  if (!p3a_service_->IsP3AEnabled() || !IsRewardsDisabled(prefs_)) {
    return;
  }
  UpdateMetricCount(creative_instance_id, kViewEventKey);
}

void NTPP3AHelperImpl::RecordClickAndMaybeLand(
    const std::string& creative_instance_id) {
  if (!p3a_service_->IsP3AEnabled() || !IsRewardsDisabled(prefs_)) {
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

void NTPP3AHelperImpl::OnP3ARotation(p3a::MetricLogType log_type,
                                     bool is_constellation) {
  if (log_type != p3a::MetricLogType::kExpress) {
    return;
  }
  ScopedDictPrefUpdate update(local_state_, GetCountDictPref(is_constellation));
  base::Value::Dict& update_dict = update.Get();

  if (!p3a_service_->IsP3AEnabled()) {
    update_dict.clear();
    return;
  }

  size_t total_active_creatives = 0;
  for (const auto [creative_instance_id, creative_value] : update_dict) {
    base::Value::Dict& creative_dict = creative_value.GetDict();
    base::Value::Dict* inflight_dict = creative_dict.FindDict(kInflightDictKey);
    DCHECK(inflight_dict);
    bool is_active_creative = false;
    for (const auto [key, value] : creative_dict) {
      if (key == kExpireTimeKey || key == kInflightDictKey) {
        continue;
      }
      int count = value.GetInt();
      inflight_dict->Set(key, count);
      RecordMetric(BuildHistogramName(creative_instance_id, key), count,
                   is_constellation);
      is_active_creative = true;
    }
    if (is_active_creative) {
      total_active_creatives++;
    }
  }
  // Always send the creative total if ads are disabled (as per spec),
  // or send the total if there were outstanding events sent
  if (IsRewardsDisabled(prefs_) || total_active_creatives > 0) {
    RecordMetric(kCreativeTotalCountHistogramName, total_active_creatives,
                 is_constellation);
  }
}

void NTPP3AHelperImpl::OnP3AMetricCycled(const std::string& histogram_name,
                                         bool is_constellation) {
  if (!histogram_name.starts_with(p3a::kCreativeMetricPrefix)) {
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

  [&]() {
    ScopedDictPrefUpdate update(local_state_,
                                GetCountDictPref(is_constellation));
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict* creative_dict =
        update_dict.FindDict(creative_instance_id);
    if (creative_dict == nullptr) {
      return;
    }
    base::Value::Dict* inflight_dict =
        creative_dict->FindDict(kInflightDictKey);
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
      creative_dict->Remove(event_type);
      // If the only elements left in the dict are expiry time and inflight
      // dict, then remove the creative dict
      if (creative_dict->size() <= 2) {
        update_dict.Remove(creative_instance_id);
      }
    }
  }();
  RemoveMetricIfInstanceDoesNotExist(histogram_name, event_type,
                                     creative_instance_id);
}

std::string NTPP3AHelperImpl::BuildHistogramName(
    const std::string& creative_instance_id,
    const std::string& event_type) {
  return base::StrCat(
      {p3a::kCreativeMetricPrefix, creative_instance_id, ".", event_type});
}

void NTPP3AHelperImpl::RecordMetric(const std::string& histogram_name,
                                    int count,
                                    bool is_constellation) {
  const int* it_count =
      std::lower_bound(kCountBuckets, std::end(kCountBuckets), count);
  int answer = it_count - kCountBuckets;
  if (use_uma_for_testing_) {
    if (!is_constellation) {
      base::UmaHistogramExactLinear(histogram_name, answer,
                                    std::size(kCountBuckets) + 1);
    }
    return;
  }
  p3a_service_->UpdateMetricValueForSingleFormat(histogram_name, answer,
                                                 is_constellation);
}

void NTPP3AHelperImpl::RemoveMetricIfInstanceDoesNotExist(
    const std::string& histogram_name,
    const std::string& event_type,
    const std::string& creative_instance_id) {
  bool creative_instance_exists =
      base::ranges::any_of(kAllCountDicts, [&](const char* dict_pref_name) {
        const auto& count_dict = local_state_->GetDict(dict_pref_name);
        const auto* creative_dict = count_dict.FindDict(creative_instance_id);
        if (creative_dict == nullptr) {
          return false;
        }
        return creative_dict->contains(event_type);
      });
  if (!creative_instance_exists) {
    p3a_service_->RemoveDynamicMetric(histogram_name);
  }
}

void NTPP3AHelperImpl::UpdateMetricCount(
    const std::string& creative_instance_id,
    const std::string& event_type) {
  const std::string histogram_name =
      BuildHistogramName(creative_instance_id, event_type);

  p3a_service_->RegisterDynamicMetric(histogram_name,
                                      p3a::MetricLogType::kExpress);

  // Perform updates for both JSON & Constellation dictionaries
  // The counts need to be monitored separately, as JSON and Constellation
  // epochs do not perfectly align.
  // TODO(djandries): Remove JSON counts once transition to Constellation
  // is complete
  for (const char* dict_pref : kAllCountDicts) {
    ScopedDictPrefUpdate update(local_state_, dict_pref);
    base::Value::Dict& update_dict = update.Get();

    base::Value::Dict* creative_instance_dict =
        update_dict.FindDict(creative_instance_id);
    if (creative_instance_dict == nullptr) {
      creative_instance_dict = update_dict.EnsureDict(creative_instance_id);
      creative_instance_dict->EnsureDict(kInflightDictKey);
    }

    const std::optional<int> current_value =
        creative_instance_dict->FindInt(event_type);

    const int count = current_value.value_or(0) + 1;

    creative_instance_dict->Set(event_type, count);
    const base::Time new_expiry_time = base::Time::Now() + kCountExpiryTime;
    creative_instance_dict->Set(kExpireTimeKey,
                                base::TimeToValue(new_expiry_time).GetString());
  }
}

void NTPP3AHelperImpl::OnLandingStartCheck(
    const std::string& creative_instance_id) {
  if (!last_tab_hostname_.has_value()) {
    return;
  }
  landing_check_timer_.Start(
      FROM_HERE, brave_ads::kPageLandAfter.Get(),
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
