/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_
#define BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/p3a/metric_log_type.h"

class PrefService;

namespace p3a {

inline constexpr char kP3AMessageNebulaNameValueSeparator[] = "=";
inline constexpr char kP3AMessageConstellationKeyValueSeparator[] = "|";
inline constexpr char kP3AMessageConstellationLayerSeparator[] = ";";

class MessageMetainfo {
 public:
  MessageMetainfo();
  ~MessageMetainfo();

  void Init(PrefService* local_state,
            std::string brave_channel,
            std::string week_of_install);

  void Update();

  const std::string& GetCountryCodeForNormalMetrics() const;

  const std::string& platform() const { return platform_; }
  const std::string& channel() const { return channel_; }
  const std::string& version() const { return version_; }
  const std::string& country_code_from_locale_raw() const {
    return country_code_from_locale_raw_;
  }
  const std::string& ref() const { return ref_; }
  base::Time date_of_install() const { return date_of_install_; }
  base::Time date_of_survey() const { return date_of_survey_; }
  int woi() const { return woi_; }

 private:
  // Used to report major/minor version numbers to reduce amount of
  // Constellation tags
  void InitVersion();

  void InitRef();

  // Ensures that country represent the big enough cohort to
  // maximize STAR recovery rate for the country code & week of install
  // attributes.
  void MaybeStripCountry();

  std::string platform_;
  std::string version_;
  std::string channel_;
  base::Time date_of_install_;
  base::Time date_of_survey_;
  int woi_;  // Week of install. Remove this occasionally and extract from
             // above.
  std::string country_code_from_timezone_;
  std::string country_code_from_locale_;
  std::string country_code_from_locale_raw_;
  // May contain 'none', a 'BRV'-prefixed refcode, or 'other'.
  std::string ref_;

  raw_ptr<PrefService, DanglingUntriaged> local_state_ = nullptr;
};

base::Value::Dict GenerateP3AMessageDict(std::string_view metric_name,
                                         uint64_t metric_value,
                                         MetricLogType log_type,
                                         const MessageMetainfo& meta,
                                         const std::string& upload_type);

std::string GenerateP3AConstellationMessage(std::string_view metric_name,
                                            uint64_t metric_value,
                                            const MessageMetainfo& meta,
                                            const std::string& upload_type,
                                            bool include_refcode,
                                            bool is_nebula);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_
