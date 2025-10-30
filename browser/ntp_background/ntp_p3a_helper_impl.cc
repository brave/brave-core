/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_service.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ntp_background_images {

namespace {

constexpr char kNewTabPageEventCountConstellationDictPref[] =
    "brave.brave_ads.p3a.ntp_event_count_constellation";
constexpr char kNewTabPageKnownCampaignsDictPref[] =
    "brave.brave_ads.p3a.ntp_known_campaigns";

constexpr int kCountBuckets[] = {0, 1, 2, 3, 8, 12, 16};

constexpr char kCreativeViewEventKey[] = "views";
constexpr char kCreativeClickEventKey[] = "clicks";
constexpr char kCreativeLandEventKey[] = "lands";
constexpr char kCreativeInteractionEventKey[] = "interaction";
constexpr char kCreativeMediaPlayEventKey[] = "media_play";
constexpr char kCreativeMedia25EventKey[] = "media_25";
constexpr char kCreativeMedia100EventKey[] = "media_100";

constexpr char kCampaignViewedEventKey[] = "viewed";
constexpr char kCampaignAwareEventKey[] = "aware";

constexpr char kCreativeTotalCountHistogramName[] =
    "creativeInstanceId.total.count";

constexpr char kInflightDictKey[] = "inflight";
constexpr char kExpireTimeKey[] = "expiry_time";

constexpr base::TimeDelta kCountExpiryTime = base::Days(30);

bool IsRewardsEnabled(PrefService* prefs) {
  return prefs->GetBoolean(brave_rewards::prefs::kEnabled);
}

std::string BuildCreativeHistogramName(const std::string& creative_instance_id,
                                       const std::string& event_type) {
  return base::StrCat(
      {p3a::kCreativeMetricPrefix, creative_instance_id, ".", event_type});
}

std::string BuildCampaignHistogramName(const std::string& campaign_id,
                                       const std::string& event_type) {
  return base::StrCat(
      {p3a::kCampaignMetricPrefix, campaign_id, ".", event_type});
}

bool CheckExpiry(const base::Time& now, const base::Value::Dict* dict) {
  if (!dict) {
    return false;
  }

  const base::Value* expiry_time_value = dict->Find(kExpireTimeKey);
  if (!expiry_time_value) {
    return false;
  }

  const auto expiry_time = base::ValueToTime(*expiry_time_value);
  return expiry_time && expiry_time < now;
}

}  // namespace

NTPP3AHelperImpl::NTPP3AHelperImpl(
    PrefService* local_state,
    p3a::P3AService* p3a_service,
    NTPBackgroundImagesService* ntp_background_images_service,
    PrefService* prefs)
    : local_state_(local_state), p3a_service_(p3a_service), prefs_(prefs) {
  DCHECK(local_state);
  DCHECK(p3a_service);
  DCHECK(prefs);
  metric_sent_subscription_ =
      p3a_service->RegisterMetricCycledCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3AMetricCycled, base::Unretained(this)));
  rotation_subscription_ =
      p3a_service->RegisterRotationCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3ARotation, base::Unretained(this)));
  if (ntp_background_images_service) {
    if (const auto* sr_data =
            ntp_background_images_service->GetSponsoredImagesData(
                /*super_referral=*/true, /*supports_rich_media=*/true)) {
      CheckLoadedCampaigns(*sr_data);
    }
    if (const auto* si_data =
            ntp_background_images_service->GetSponsoredImagesData(
                /*super_referral=*/false, /*supports_rich_media=*/true)) {
      CheckLoadedCampaigns(*si_data);
    }
    ntp_background_images_service_observation_.Observe(
        ntp_background_images_service);
  }
}

NTPP3AHelperImpl::~NTPP3AHelperImpl() = default;

void NTPP3AHelperImpl::RecordView(const std::string& creative_instance_id,
                                  const std::string& campaign_id) {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }

  ScopedDictPrefUpdate update(local_state_, kNewTabPageKnownCampaignsDictPref);
  auto* campaign_dict = update->FindDict(campaign_id);
  if (campaign_dict &&
      !campaign_dict->FindBool(kCampaignViewedEventKey).has_value()) {
    campaign_dict->Set(kCampaignViewedEventKey, true);
    UpdateCampaignMetric(campaign_id, kCampaignViewedEventKey);
  }

  if (IsRewardsEnabled(prefs_)) {
    return;
  }
  UpdateMetricCount(creative_instance_id, kCreativeViewEventKey);
}

void NTPP3AHelperImpl::RecordNewTabPageAdEvent(
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
    const std::string& creative_instance_id) {
  if (!p3a_service_->IsP3AEnabled() || IsRewardsEnabled(prefs_)) {
    return;
  }

  switch (mojom_ad_event_type) {
    case brave_ads::mojom::NewTabPageAdEventType::kServedImpression:
    case brave_ads::mojom::NewTabPageAdEventType::kViewedImpression: {
      // Served impressions are handled by the ads component. Viewed impressions
      // are handled in `RecordView` which is called when a sponsored ad is be
      // displayed.
      NOTREACHED() << "Unexpected mojom::NewTabPageAdEventType: "
                   << base::to_underlying(mojom_ad_event_type);
    }

    case brave_ads::mojom::NewTabPageAdEventType::kClicked: {
      UpdateMetricCount(creative_instance_id, kCreativeClickEventKey);
      last_clicked_creative_instance_id_ = creative_instance_id;
      break;
    }

    case brave_ads::mojom::NewTabPageAdEventType::kInteraction: {
      UpdateMetricCount(creative_instance_id, kCreativeInteractionEventKey);
      break;
    }

    case brave_ads::mojom::NewTabPageAdEventType::kMediaPlay: {
      UpdateMetricCount(creative_instance_id, kCreativeMediaPlayEventKey);
      break;
    }

    case brave_ads::mojom::NewTabPageAdEventType::kMedia25: {
      UpdateMetricCount(creative_instance_id, kCreativeMedia25EventKey);
      break;
    }

    case brave_ads::mojom::NewTabPageAdEventType::kMedia100: {
      UpdateMetricCount(creative_instance_id, kCreativeMedia100EventKey);
      break;
    }
  }
}

void NTPP3AHelperImpl::OnNavigationDidFinish(const GURL& url) {
  last_url_ = url;

  MaybeLand(url);
}

void NTPP3AHelperImpl::OnP3ARotation(p3a::MetricLogType log_type) {
  if (log_type != p3a::MetricLogType::kExpress) {
    return;
  }

  CleanOldCampaignsAndCreatives();

  ScopedDictPrefUpdate update(local_state_,
                              kNewTabPageEventCountConstellationDictPref);
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
      RecordCreativeMetric(
          BuildCreativeHistogramName(creative_instance_id, key), count);
      is_active_creative = true;
    }
    if (is_active_creative) {
      total_active_creatives++;
    }
  }
  // Always send the creative total if ads are disabled (as per spec),
  // or send the total if there were outstanding events sent
  if (!IsRewardsEnabled(prefs_) || total_active_creatives > 0) {
    RecordCreativeMetric(kCreativeTotalCountHistogramName,
                         total_active_creatives);
  }
}

void NTPP3AHelperImpl::OnP3AMetricCycled(const std::string& histogram_name) {
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
                                kNewTabPageEventCountConstellationDictPref);
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

void NTPP3AHelperImpl::CleanOldCampaignsAndCreatives() {
  ScopedDictPrefUpdate update(local_state_, kNewTabPageKnownCampaignsDictPref);

  if (!p3a_service_->IsP3AEnabled()) {
    update->clear();
    return;
  }

  const auto now = base::Time::Now();

  for (auto it = update->begin(); it != update->end();) {
    // wrap this in an anon func
    const auto& campaign_id = it->first;
    const auto* campaign_dict = it->second.GetIfDict();

    if (!CheckExpiry(now, campaign_dict)) {
      it++;
      continue;
    }

    p3a_service_->RemoveDynamicMetric(
        BuildCampaignHistogramName(campaign_id, kCampaignViewedEventKey));
    p3a_service_->RemoveDynamicMetric(
        BuildCampaignHistogramName(campaign_id, kCampaignAwareEventKey));
    it = update->erase(it);
  }

  ScopedDictPrefUpdate creative_update(
      local_state_, kNewTabPageEventCountConstellationDictPref);

  for (auto it = creative_update->begin(); it != creative_update->end();) {
    const auto& creative_instance_id = it->first;
    base::Value::Dict* creative_instance_dict = it->second.GetIfDict();
    if (!CheckExpiry(now, creative_instance_dict)) {
      it++;
      continue;
    }
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeClickEventKey));
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeViewEventKey));
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeLandEventKey));
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeInteractionEventKey));
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeMediaPlayEventKey));
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeMedia25EventKey));
    p3a_service_->RemoveDynamicMetric(BuildCreativeHistogramName(
        creative_instance_id, kCreativeMedia100EventKey));
    it = creative_update->erase(it);
  }
}

void NTPP3AHelperImpl::MaybeLand(const GURL& url) {
  if (!last_clicked_creative_instance_id_) {
    // The user did not click on a new tab page ad, so there is no need to check
    // for a page landing.
    return;
  }

  page_land_timer_.Start(
      FROM_HERE, brave_ads::kPageLandAfter.Get(),
      base::BindOnce(&NTPP3AHelperImpl::MaybeLandCallback,
                     base::Unretained(this),
                     *last_clicked_creative_instance_id_, url));

  last_clicked_creative_instance_id_.reset();
}

void NTPP3AHelperImpl::MaybeLandCallback(
    const std::string& creative_instance_id,
    const GURL& url) {
  if (last_url_ && last_url_->host() == url.host()) {
    UpdateMetricCount(creative_instance_id, kCreativeLandEventKey);
  }
}

void NTPP3AHelperImpl::RecordCreativeMetric(const std::string& histogram_name,
                                            int count) {
  const int* it_count =
      std::lower_bound(kCountBuckets, std::end(kCountBuckets), count);
  int answer = it_count - kCountBuckets;
  base::UmaHistogramExactLinear(histogram_name, answer,
                                std::size(kCountBuckets) + 1);
}

void NTPP3AHelperImpl::RemoveMetricIfInstanceDoesNotExist(
    const std::string& histogram_name,
    const std::string& event_type,
    const std::string& creative_instance_id) {
  const auto& count_dict =
      local_state_->GetDict(kNewTabPageEventCountConstellationDictPref);
  const auto* creative_dict = count_dict.FindDict(creative_instance_id);
  bool creative_instance_exists =
      creative_dict && creative_dict->contains(event_type);

  if (!creative_instance_exists) {
    p3a_service_->RemoveDynamicMetric(histogram_name);
  }
}

void NTPP3AHelperImpl::UpdateMetricCount(
    const std::string& creative_instance_id,
    const std::string& event_type) {
  const std::string histogram_name =
      BuildCreativeHistogramName(creative_instance_id, event_type);

  p3a_service_->RegisterDynamicMetric(histogram_name,
                                      p3a::MetricLogType::kExpress);

  ScopedDictPrefUpdate update(local_state_,
                              kNewTabPageEventCountConstellationDictPref);
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

void NTPP3AHelperImpl::UpdateCampaignMetric(const std::string& campaign_id,
                                            const std::string& event_type) {
  const std::string histogram_name =
      BuildCampaignHistogramName(campaign_id, event_type);

  p3a_service_->RegisterDynamicMetric(histogram_name,
                                      p3a::MetricLogType::kExpress);
  base::UmaHistogramBoolean(histogram_name, true);
}

void NTPP3AHelperImpl::OnSponsoredImagesDataDidUpdate(
    NTPSponsoredImagesData* data) {
  CheckLoadedCampaigns(*data);
}

void NTPP3AHelperImpl::CheckLoadedCampaigns(
    const NTPSponsoredImagesData& data) {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }

  ScopedDictPrefUpdate update(local_state_, kNewTabPageKnownCampaignsDictPref);
  for (const auto& campaign : data.campaigns) {
    if (update->FindDict(campaign.campaign_id)) {
      continue;
    }
    base::Value::Dict campaign_dict;
    campaign_dict.Set(kExpireTimeKey,
                      base::TimeToValue(base::Time::Now() + kCountExpiryTime));
    update->Set(campaign.campaign_id, std::move(campaign_dict));
    UpdateCampaignMetric(campaign.campaign_id, kCampaignAwareEventKey);
  }
}

}  // namespace ntp_background_images
