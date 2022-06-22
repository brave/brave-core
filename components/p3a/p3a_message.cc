/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_message.h"

#include <algorithm>
#include <array>

#include "base/containers/flat_set.h"
#include "base/i18n/timezone.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/version_info/version_info.h"

namespace brave {

MessageMetainfo::MessageMetainfo() = default;
MessageMetainfo::~MessageMetainfo() = default;

constexpr std::size_t kP3AStarAttributeCount = 8;

void MessageMetainfo::Init(PrefService* local_state,
                           std::string channel,
                           std::string week_of_install) {
  platform = brave_stats::GetPlatformIdentifier();
  this->channel = channel;
  version = version_info::GetBraveVersionWithoutChromiumMajorVersion();

  if (!week_of_install.empty()) {
    date_of_install = brave_stats::GetYMDAsDate(week_of_install);
  } else {
    date_of_install = base::Time::Now();
  }
  woi = brave_stats::GetIsoWeekNumber(date_of_install);

  country_code = base::ToUpperASCII(base::CountryCodeForCurrentTimezone());
  refcode = local_state->GetString(kReferralPromoCode);
  MaybeStripRefcodeAndCountry();

  Update();

  VLOG(2) << "Message meta: " << platform << " " << channel << " " << version
          << " " << woi << " " << wos << " " << country_code << " " << refcode;
}

void MessageMetainfo::Update() {
  date_of_survey = base::Time::Now();
  wos = brave_stats::GetIsoWeekNumber(date_of_survey);
}

void MessageMetainfo::MaybeStripRefcodeAndCountry() {
  const std::string& country = country_code;
  constexpr char kRefcodeNone[] = "none";
  constexpr char kCountryOther[] = "other";

  static base::flat_set<std::string> const kLinuxCountries(
      {"US", "FR", "DE", "GB", "IN", "BR", "PL", "NL", "ES", "CA", "IT", "AU",
       "MX", "CH", "RU", "ZA", "SE", "BE", "JP"});

  static base::flat_set<std::string> const kNotableCountries(
      {"US", "FR", "PH", "GB", "IN", "DE", "BR", "CA", "IT", "ES", "NL", "MX",
       "AU", "RU", "JP", "PL", "ID", "KR", "AR"});

  // Always strip the refcode.
  // We no longer need to partition P3A data with that key.
  refcode = kRefcodeNone;

  if (platform == "linux-bc") {
    // If we have more than 3/0.05 = 60 users in a country for
    // a week of install, we can send country.
    if (kLinuxCountries.count(country) == 0) {
      country_code = kCountryOther;
    }
  } else {
    // Now the minimum platform is MacOS at ~3%, so cut off for a group under
    // here becomes 3/(0.05*0.03) = 2000.
    if (kNotableCountries.count(country) == 0) {
      country_code = kCountryOther;
    }
  }
}

base::Value GenerateP3AMessageDict(base::StringPiece metric_name,
                                   uint64_t metric_value,
                                   const MessageMetainfo& meta) {
  base::Value result(base::Value::Type::DICTIONARY);

  // Find out years of install and survey.
  base::Time::Exploded exploded;
  meta.date_of_survey.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  result.SetIntKey("yos", exploded.year);

  meta.date_of_install.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  result.SetIntKey("yoi", exploded.year);

  // Fill meta.
  result.SetStringKey("country_code", meta.country_code);
  result.SetStringKey("platform", meta.platform);
  result.SetStringKey("version", meta.version);
  result.SetStringKey("channel", meta.channel);
  result.SetIntKey("woi", meta.woi);
  result.SetIntKey("wos", meta.wos);

  // Set the metric
  result.SetStringKey("metric_name", metric_name);
  result.SetIntKey("metric_value", metric_value);

  return result;
}

std::string GenerateP3AStarMessage(base::StringPiece metric_name,
                                   uint64_t metric_value,
                                   const MessageMetainfo& meta) {
  base::Time::Exploded exploded;
  meta.date_of_install.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);

  std::array<std::array<std::string, 2>, kP3AStarAttributeCount> attributes = {{
      {"metric_name", std::string(metric_name)},
      {"metric_value", base::NumberToString(metric_value)},
      {"version", meta.version},
      {"yoi", base::NumberToString(exploded.year)},
      {"channel", meta.channel},
      {"platform", meta.platform},
      {"country_code", meta.country_code},
      {"woi", base::NumberToString(meta.woi)},
  }};

  std::array<std::string, kP3AStarAttributeCount> serialized_attributes;

  std::transform(attributes.begin(), attributes.end(),
                 serialized_attributes.begin(), [](auto& attr) -> std::string {
                   return base::JoinString(attr,
                                           kP3AMessageStarKeyValueSeparator);
                 });

  return base::JoinString(serialized_attributes, kP3AMessageStarLayerSeparator);
}

}  // namespace brave
