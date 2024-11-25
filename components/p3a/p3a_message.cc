/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_message.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/i18n/timezone.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/p3a/uploader.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/pref_service.h"

#if !BUILDFLAG(IS_IOS)
#include "brave/components/brave_referrals/common/pref_names.h"
#endif  // !BUILDFLAG(IS_IOS)

namespace p3a {

namespace {

constexpr char kMetricNameAttributeName[] = "metric_name";
constexpr char kMetricValueAttributeName[] = "metric_value";
constexpr char kMetricNameAndValueAttributeName[] = "metric_name_and_value";
constexpr char kPlatformAttributeName[] = "platform";
constexpr char kChannelAttributeName[] = "channel";
constexpr char kYosAttributeName[] = "yos";
constexpr char kWosAttributeName[] = "wos";
constexpr char kMosAttributeName[] = "mos";
constexpr char kWoiAttributeName[] = "woi";
constexpr char kYoiAttributeName[] = "yoi";
constexpr char kCountryCodeAttributeName[] = "country_code";
constexpr char kVersionAttributeName[] = "version";
constexpr char kCadenceAttributeName[] = "cadence";
constexpr char kRefAttributeName[] = "ref";

constexpr char kSlowCadence[] = "slow";
constexpr char kTypicalCadence[] = "typical";
constexpr char kExpressCadence[] = "express";

constexpr char kOrganicRefPrefix[] = "BRV";
constexpr char kRefNone[] = "none";
constexpr char kRefOther[] = "other";

constexpr auto kLinuxCountries = base::MakeFixedFlatSet<std::string_view>(
    {"US", "FR", "DE", "GB", "IN", "BR", "PL", "NL", "ES", "CA",
     "IT", "AU", "MX", "CH", "RU", "ZA", "SE", "BE", "JP", "AT"});

constexpr auto kNotableCountries = base::MakeFixedFlatSet<std::string_view>(
    {"US", "FR", "PH", "GB", "IN", "DE", "BR", "CA", "IT", "ES",
     "NL", "MX", "AU", "RU", "JP", "PL", "ID", "KR", "AR", "AT"});

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
  result.Set(kPlatformAttributeName, meta.platform());
  result.Set(kChannelAttributeName, meta.channel());
  // Set the metric
  result.Set(kMetricNameAttributeName, metric_name);
  result.Set(kMetricValueAttributeName, static_cast<int>(metric_value));

  if (upload_type == kP3ACreativeUploadType) {
    return result;
  }

  base::Time date_of_install_monday =
      brave_stats::GetLastMondayTime(meta.date_of_install());
  base::Time date_of_survey = meta.date_of_survey();

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
  result.Set(kCountryCodeAttributeName, meta.GetCountryCodeForNormalMetrics());
  result.Set(kVersionAttributeName, meta.version());
  result.Set(kWoiAttributeName, meta.woi());

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
                                            const std::string& upload_type,
                                            bool include_refcode,
                                            bool is_nebula) {
  base::Time::Exploded exploded;
  meta.date_of_install().LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);

  std::vector<std::array<std::string, 2>> attributes;

  std::string metric_name_str = std::string(metric_name);
  std::string metric_value_str = base::NumberToString(metric_value);

  if (!is_nebula) {
    attributes = {{{kMetricNameAttributeName, metric_name_str},
                   {kMetricValueAttributeName, metric_value_str}}};
  } else {
    attributes = {{kMetricNameAndValueAttributeName,
                   base::JoinString({metric_name_str, metric_value_str},
                                    kP3AMessageNebulaNameValueSeparator)}};
  }

  bool is_creative = upload_type == kP3ACreativeUploadType;

  if (!is_creative) {
    attributes.push_back({kVersionAttributeName, meta.version()});
    attributes.push_back(
        {kYoiAttributeName, base::NumberToString(exploded.year)});
  }

  attributes.push_back({kChannelAttributeName, meta.channel()});
  attributes.push_back({kPlatformAttributeName, meta.platform()});

  if (is_creative) {
    attributes.push_back(
        {kCountryCodeAttributeName, meta.country_code_from_locale_raw()});
  } else {
    attributes.push_back(
        {kCountryCodeAttributeName, meta.GetCountryCodeForNormalMetrics()});
    attributes.push_back({kWoiAttributeName, base::NumberToString(meta.woi())});
  }

  if (include_refcode) {
    attributes.push_back({kRefAttributeName, meta.ref()});
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
  local_state_ = local_state;
  platform_ = brave_stats::GetPlatformIdentifier();
  channel_ = brave_channel;
  InitVersion();
  InitRef();

  if (!week_of_install.empty()) {
    date_of_install_ = brave_stats::GetYMDAsDate(week_of_install);
  } else {
    date_of_install_ = base::Time::Now();
  }
  woi_ = brave_stats::GetIsoWeekNumber(date_of_install_);

  country_code_from_timezone_ =
      base::ToUpperASCII(base::CountryCodeForCurrentTimezone());
  country_code_from_locale_raw_ = brave_l10n::GetDefaultISOCountryCodeString();
  country_code_from_locale_ = country_code_from_locale_raw_;

  MaybeStripCountry();

  Update();

  VLOG(2) << "Message meta: " << platform_ << " " << channel_ << " " << version_
          << " " << woi_ << " " << country_code_from_timezone_ << " "
          << country_code_from_locale_ << " " << ref_;
}

void MessageMetainfo::Update() {
  date_of_survey_ = base::Time::Now();
  InitRef();
}

void MessageMetainfo::InitVersion() {
  std::string full_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  std::vector<std::string> version_numbers = base::SplitString(
      full_version, ".", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);
  if (version_numbers.size() <= 2) {
    version_ = full_version;
  } else {
    version_ = base::StrCat({version_numbers[0], ".", version_numbers[1]});
  }
}

void MessageMetainfo::InitRef() {
  std::string referral_code;
#if !BUILDFLAG(IS_IOS)
  if (local_state_ && local_state_->HasPrefPath(kReferralPromoCode)) {
    referral_code = local_state_->GetString(kReferralPromoCode);
  }
#endif  // !BUILDFLAG(IS_IOS)
  if (referral_code.empty()) {
    ref_ = kRefNone;
  } else if (referral_code.starts_with(kOrganicRefPrefix)) {
    ref_ = referral_code;
  } else {
    ref_ = kRefOther;
  }
}

void MessageMetainfo::MaybeStripCountry() {
  constexpr char kCountryOther[] = "other";

  if (platform_ == "linux-bc") {
    // If we have more than 3/0.05 = 60 users in a country for
    // a week of install, we can send country.
    if (!kLinuxCountries.contains(country_code_from_timezone_)) {
      country_code_from_timezone_ = kCountryOther;
    }
  } else {
    // Now the minimum platform is MacOS at ~3%, so cut off for a group under
    // here becomes 3/(0.05*0.03) = 2000.
    if (!kNotableCountries.contains(country_code_from_timezone_)) {
      country_code_from_timezone_ = kCountryOther;
    }
    if (!kNotableCountries.contains(country_code_from_locale_)) {
      country_code_from_locale_ = kCountryOther;
    }
  }
}

const std::string& MessageMetainfo::GetCountryCodeForNormalMetrics() const {
#if BUILDFLAG(IS_IOS)
  return country_code_from_locale_;
#else
  return country_code_from_timezone_;
#endif  // BUILDFLAG(IS_IOS)
}

}  // namespace p3a
