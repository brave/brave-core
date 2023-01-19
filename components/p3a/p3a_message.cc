/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_message.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/p3a/brave_p3a_uploader.h"
#include "crypto/sha2.h"

namespace brave {

MessageMetainfo::MessageMetainfo() = default;
MessageMetainfo::~MessageMetainfo() = default;

base::Value::Dict GenerateP3AMessageDict(base::StringPiece metric_name,
                                         uint64_t metric_value,
                                         const MessageMetainfo& meta,
                                         const std::string& upload_type) {
  base::Value::Dict result;

  // Fill basic meta.
  result.Set("platform", meta.platform);
  result.Set("channel", meta.channel);
  // Set the metric
  result.Set("metric_name", metric_name);
  result.Set("metric_value", static_cast<int>(metric_value));

  if (upload_type == kP3ACreativeUploadType) {
    return result;
  }

  // Get last monday for the dates so that the years of survey/install correctly
  // match the ISO weeks of survey/install.
  // i.e. date of survey = Sunday, January 1, 2023 should result in
  // yos = 2022 and wos = 52 since that date falls on the last ISO week of the
  // previous year.
  base::Time date_of_survey_monday =
      brave_stats::GetLastMondayTime(meta.date_of_survey);
  base::Time date_of_install_monday =
      brave_stats::GetLastMondayTime(meta.date_of_install);

  // Find out years of install and survey.
  base::Time::Exploded exploded;
  date_of_survey_monday.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  result.Set("yos", exploded.year);

  date_of_install_monday.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  result.Set("yoi", exploded.year);

  // Fill meta.
  result.Set("country_code", meta.country_code);
  result.Set("version", meta.version);
  result.Set("woi", meta.woi);
  result.Set("wos", meta.wos);

  return result;
}

void MaybeStripCountry(MessageMetainfo* meta) {
  const std::string& country = meta->country_code;
  constexpr char kCountryOther[] = "other";

  static base::flat_set<std::string> const kLinuxCountries(
      {"US", "FR", "DE", "GB", "IN", "BR", "PL", "NL", "ES", "CA", "IT", "AU",
       "MX", "CH", "RU", "ZA", "SE", "BE", "JP"});

  static base::flat_set<std::string> const kNotableCountries(
      {"US", "FR", "PH", "GB", "IN", "DE", "BR", "CA", "IT", "ES", "NL", "MX",
       "AU", "RU", "JP", "PL", "ID", "KR", "AR"});

  DCHECK(meta);

  if (meta->platform == "linux-bc") {
    // If we have more than 3/0.05 = 60 users in a country for
    // a week of install, we can send country.
    if (kLinuxCountries.count(country) == 0) {
      meta->country_code = kCountryOther;
    }
  } else {
    // Now the minimum platform is MacOS at ~3%, so cut off for a group under
    // here becomes 3/(0.05*0.03) = 2000.
    if (kNotableCountries.count(country) == 0) {
      meta->country_code = kCountryOther;
    }
  }
}

}  // namespace brave
