/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/reports/reports.h"

#include "base/check.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

Reports::Reports(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Reports::~Reports() = default;

std::string Reports::GenerateSettingsEventReport() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

  writer.StartObject();

  writer.String("data");
  writer.StartObject();

  writer.String("type");
  writer.String("settings");

  writer.String("timestamp");
  const base::Time time = base::Time::Now();
  const std::string timestamp = LongFormatFriendlyDateAndTime(time, false);
  writer.String(timestamp.c_str());

  writer.String("settings");
  writer.StartObject();

  writer.String("locale");
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  writer.String(locale.c_str());

  writer.String("notifications");
  writer.StartObject();

  writer.String("shouldShow");
  auto should_show = ads_->get_ads_client()->ShouldShowNotifications();
  writer.Bool(should_show);

  writer.EndObject();

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
