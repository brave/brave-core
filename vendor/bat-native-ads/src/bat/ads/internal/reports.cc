/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/reports.h"
#include "bat/ads/internal/time.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/search_providers.h"
#include "bat/ads/ad_notification_info.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace ads {

Reports::Reports(
    AdsImpl* ads)
    : is_first_run_(true),
      ads_(ads) {
  DCHECK(ads_);
}

Reports::~Reports() = default;

std::string Reports::GenerateAdNotificationEventReport(
    const AdNotificationInfo& info,
    const AdNotificationEventType event_type) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  const std::string timestamp = Time::Timestamp();

  writer.StartObject();

  if (is_first_run_) {
    is_first_run_ = false;

    writer.String("data");
    writer.StartObject();

    writer.String("type");
    writer.String("restart");

    writer.String("timestamp");
    writer.String(timestamp.c_str());

    writer.EndObject();
  }

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("notify");

  writer.String("timestamp");
  writer.String(timestamp.c_str());

  writer.String("eventType");
  switch (event_type) {
    case AdNotificationEventType::kViewed: {
      writer.String("generated");
      break;
    }

    case AdNotificationEventType::kClicked: {
      writer.String("clicked");
      break;
    }

    case AdNotificationEventType::kDismissed: {
      writer.String("dismissed");
      break;
    }

    case AdNotificationEventType::kTimedOut: {
      writer.String("timed out");
      break;
    }
  }

  writer.String("classifications");
  writer.StartArray();
  auto classifications =
      helper::Classification::GetClassifications(info.category);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  writer.String("adCatalog");
  if (ads_->IsCreativeSetFromSampleCatalog(info.creative_set_id)) {
    writer.String("sample-catalog");
  } else {
    writer.String(info.creative_set_id.c_str());
  }

  writer.String("targetUrl");
  writer.String(info.target_url.c_str());

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateConfirmationEventReport(
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type) const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("confirmation");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.String("creativeInstanceId");
  writer.String(creative_instance_id.c_str());

  writer.String("confirmationType");
  writer.String(std::string(confirmation_type).c_str());

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateLoadEventReport(
    const LoadInfo& info) const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("load");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.String("tabType");
  if (SearchProviders::IsSearchEngine(info.tab_url)) {
    writer.String("search");
  } else {
    writer.String("click");
  }

  writer.String("tabUrl");
  writer.String(info.tab_url.c_str());

  writer.String("tabClassification");
  writer.StartArray();
  auto classifications =
      helper::Classification::GetClassifications(info.tab_classification);
  for (const auto& classification : classifications) {
    writer.String(classification.c_str());
  }
  writer.EndArray();

  auto page_score_cache = ads_->GetPageScoreCache();
  auto cached_page_score = page_score_cache.find(info.tab_url);
  if (cached_page_score != page_score_cache.end()) {
    writer.String("pageScore");
    writer.StartArray();
    for (const auto& page_score : cached_page_score->second) {
      writer.Double(page_score);
    }
    writer.EndArray();
  }

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateBackgroundEventReport() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("background");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateForegroundEventReport() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("foreground");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateBlurEventReport(
    const BlurInfo& info) const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("blur");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateDestroyEventReport(
    const DestroyInfo& info) const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("destroy");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateFocusEventReport(
    const FocusInfo& info) const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("focus");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.String("tabId");
  writer.Int(info.tab_id);

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

std::string Reports::GenerateSettingsEventReport() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("settings");

  writer.String("timestamp");
  const std::string timestamp = Time::Timestamp();
  writer.String(timestamp.c_str());

  writer.String("settings");
  writer.StartObject();

  writer.String("locale");
  auto locale = ads_->get_ads_client()->GetLocale();
  writer.String(locale.c_str());

  writer.String("notifications");
  writer.StartObject();

  writer.String("shouldShow");
  auto should_show = ads_->get_ads_client()->ShouldShowNotifications();
  writer.Bool(should_show);

  writer.EndObject();

  writer.String("userModelLanguage");
  auto user_model_language = ads_->get_client()->GetUserModelLanguage();
  writer.String(user_model_language.c_str());

  writer.String("adsPerDay");
  auto ads_per_day = ads_->get_ads_client()->GetAdsPerDay();
  writer.Uint64(ads_per_day);

  writer.String("adsPerHour");
  auto ads_per_hour = ads_->get_ads_client()->GetAdsPerHour();
  writer.Uint64(ads_per_hour);

  writer.EndObject();

  writer.EndObject();

  writer.EndObject();

  return buffer.GetString();
}

}  // namespace ads
