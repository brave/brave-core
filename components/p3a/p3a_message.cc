/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_message.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/i18n/timezone.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/l10n/common/country_code_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/p3a/uploader.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/pref_service.h"

namespace p3a {

namespace {
const char kMetricNameAttributeName[] = "metric_name";
const char kMetricValueAttributeName[] = "metric_value";
const char kPlatformAttributeName[] = "platform";
const char kChannelAttributeName[] = "channel";
const char kYosAttributeName[] = "yos";
const char kWosAttributeName[] = "wos";
const char kMosAttributeName[] = "mos";
const char kWoiAttributeName[] = "woi";
const char kYoiAttributeName[] = "yoi";
const char kCountryCodeAttributeName[] = "country_code";
const char kVersionAttributeName[] = "version";
const char kCadenceAttributeName[] = "cadence";

const char kSlowCadence[] = "slow";
const char kTypicalCadence[] = "typical";
const char kExpressCadence[] = "express";

}  // namespace

MessageMetainfo::MessageMetainfo() = default;
MessageMetainfo::~MessageMetainfo() = default;

base::Value::Dict GenerateP3AMessageDict(std::string_view metric_name,
                                         uint64_t metric_value,
                                         MetricLogType log_type,
                                         const MessageMetainfo& meta,
                                         const std::string& upload_type) {
  base::Value::Dict result;

  // Fill basic meta.
  result.Set(kPlatformAttributeName, meta.platform);
  result.Set(kChannelAttributeName, meta.channel);
  // Set the metric
  result.Set(kMetricNameAttributeName, metric_name);
  result.Set(kMetricValueAttributeName, static_cast<int>(metric_value));

  if (upload_type == kP3ACreativeUploadType) {
    return result;
  }

  base::Time date_of_install_monday =
      brave_stats::GetLastMondayTime(meta.date_of_install);
  base::Time date_of_survey = meta.date_of_survey;

  if (log_type != MetricLogType::kSlow) {
    // Get last monday for the date so that the years of survey/install
    // correctly match the ISO weeks of survey/install. i.e. date of survey =
    // Sunday, January 1, 2023 should result in yos = 2022 and wos = 52 since
    // that date falls on the last ISO week of the previous year.
    date_of_survey = brave_stats::GetLastMondayTime(date_of_survey);
  }

  // Find out years of install and survey.
  base::Time::Exploded survey_exploded;
  base::Time::Exploded install_exploded;
  date_of_survey.LocalExplode(&survey_exploded);
  date_of_install_monday.LocalExplode(&install_exploded);

  DCHECK_GE(survey_exploded.year, 999);
  result.Set(kYosAttributeName, survey_exploded.year);

  DCHECK_GE(install_exploded.year, 999);
  result.Set(kYoiAttributeName, install_exploded.year);

  // Fill meta.
  result.Set(kCountryCodeAttributeName, meta.country_code_from_timezone);
  result.Set(kVersionAttributeName, meta.version);
  result.Set(kWoiAttributeName, meta.woi);

  if (log_type == MetricLogType::kSlow) {
    result.Set(kMosAttributeName, survey_exploded.month);
  } else {
    result.Set(kWosAttributeName,
               brave_stats::GetIsoWeekNumber(date_of_survey));
  }

  std::string cadence;
  switch (log_type) {
    case MetricLogType::kSlow:
      cadence = kSlowCadence;
      break;
    case MetricLogType::kTypical:
      cadence = kTypicalCadence;
      break;
    case MetricLogType::kExpress:
      cadence = kExpressCadence;
      break;
  }
  result.Set(kCadenceAttributeName, cadence);

  return result;
}

std::string GenerateP3AConstellationMessage(std::string_view metric_name,
                                            uint64_t metric_value,
                                            const MessageMetainfo& meta,
                                            const std::string& upload_type) {
  base::Time::Exploded exploded;
  meta.date_of_install.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);

  std::vector<std::array<std::string, 2>> attributes;

  if (upload_type == kP3ACreativeUploadType) {
    attributes = {{
        {kMetricNameAttributeName, std::string(metric_name)},
        {kMetricValueAttributeName, base::NumberToString(metric_value)},
        {kChannelAttributeName, meta.channel},
        {kPlatformAttributeName, meta.platform},
        {kCountryCodeAttributeName, meta.country_code_from_locale},
    }};
  } else {
    attributes = {{
        {kMetricNameAttributeName, std::string(metric_name)},
        {kMetricValueAttributeName, base::NumberToString(metric_value)},
        {kVersionAttributeName, meta.version},
        {kYoiAttributeName, base::NumberToString(exploded.year)},
        {kChannelAttributeName, meta.channel},
        {kPlatformAttributeName, meta.platform},
        {kCountryCodeAttributeName, meta.country_code_from_timezone},
        {kWoiAttributeName, base::NumberToString(meta.woi)},
    }};
  }

  std::vector<std::string> serialized_attributes(attributes.size());

  std::transform(attributes.begin(), attributes.end(),
                 serialized_attributes.begin(), [](auto& attr) -> std::string {
                   return base::JoinString(
                       attr, kP3AMessageConstellationKeyValueSeparator);
                 });

  return base::JoinString(serialized_attributes,
                          kP3AMessageConstellationLayerSeparator);
}

void MessageMetainfo::Init(PrefService* local_state,
                           std::string brave_channel,
                           std::string week_of_install) {
  platform = brave_stats::GetPlatformIdentifier();
  channel = brave_channel;
  InitVersion();

  if (!week_of_install.empty()) {
    date_of_install = brave_stats::GetYMDAsDate(week_of_install);
  } else {
    date_of_install = base::Time::Now();
  }
  woi = brave_stats::GetIsoWeekNumber(date_of_install);

  country_code_from_timezone =
      base::ToUpperASCII(base::CountryCodeForCurrentTimezone());
  if (local_state->FindPreference(brave_l10n::prefs::kCountryCode)) {
    // Since the country code pref is not available in unit tests,
    // only load it if it's available.
    country_code_from_locale = brave_l10n::GetCountryCode(local_state);
  }
  MaybeStripCountry();

  Update();

  VLOG(2) << "Message meta: " << platform << " " << channel << " " << version
          << " " << woi << " " << country_code_from_timezone << " "
          << country_code_from_locale;
}

void MessageMetainfo::Update() {
  date_of_survey = base::Time::Now();
}

void MessageMetainfo::InitVersion() {
  std::string full_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  std::vector<std::string> version_numbers = base::SplitString(
      full_version, ".", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);
  if (version_numbers.size() <= 2) {
    version = full_version;
  } else {
    version = base::StrCat({version_numbers[0], ".", version_numbers[1]});
  }
}

void MessageMetainfo::MaybeStripCountry() {
  constexpr char kCountryOther[] = "other";

  static base::flat_set<std::string> const kLinuxCountries(
      {"US", "FR", "DE", "GB", "IN", "BR", "PL", "NL", "ES", "CA", "IT", "AU",
       "MX", "CH", "RU", "ZA", "SE", "BE", "JP"});

  static base::flat_set<std::string> const kNotableCountries(
      {"US", "FR", "PH", "GB", "IN", "DE", "BR", "CA", "IT", "ES", "NL", "MX",
       "AU", "RU", "JP", "PL", "ID", "KR", "AR"});

  if (platform == "linux-bc") {
    // If we have more than 3/0.05 = 60 users in a country for
    // a week of install, we can send country.
    if (kLinuxCountries.count(country_code_from_timezone) == 0) {
      country_code_from_timezone = kCountryOther;
    }
  } else {
    // Now the minimum platform is MacOS at ~3%, so cut off for a group under
    // here becomes 3/(0.05*0.03) = 2000.
    if (kNotableCountries.count(country_code_from_timezone) == 0) {
      country_code_from_timezone = kCountryOther;
    }
  }
}

}  // namespace p3a
