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
#include "brave/components/brave_prochlo/prochlo_message.pb.h"
#include "crypto/sha2.h"

namespace brave {

MessageMetainfo::MessageMetainfo() = default;
MessageMetainfo::~MessageMetainfo() = default;

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

void MaybeStripRefcodeAndCountry(MessageMetainfo* meta) {
  const std::string& country = meta->country_code;
  constexpr char kRefcodeNone[] = "none";
  constexpr char kCountryOther[] = "other";

  static base::flat_set<std::string> const kLinuxCountries(
      {"US", "FR", "DE", "GB", "IN", "BR", "PL", "NL", "ES", "CA", "IT", "AU",
       "MX", "CH", "RU", "ZA", "SE", "BE", "JP"});

  static base::flat_set<std::string> const kNotableCountries(
      {"US", "FR", "PH", "GB", "IN", "DE", "BR", "CA", "IT", "ES", "NL", "MX",
       "AU", "RU", "JP", "PL", "ID", "KR", "AR"});

  DCHECK(meta);

  // Always strip the refcode.
  // We no longer need to partition P3A data with that key.
  meta->refcode = kRefcodeNone;

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
