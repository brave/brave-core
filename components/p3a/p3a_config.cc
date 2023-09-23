/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_config.h"

#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/switches.h"

namespace p3a {

namespace {

constexpr uint64_t kDefaultUploadIntervalSeconds = 60;  // 1 minute.

base::TimeDelta MaybeOverrideTimeDeltaFromCommandLine(
    base::CommandLine* cmdline,
    const char* switch_name,
    base::TimeDelta default_config_value) {
  base::TimeDelta result = std::move(default_config_value);
  if (cmdline->HasSwitch(switch_name)) {
    std::string seconds_str = cmdline->GetSwitchValueASCII(switch_name);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      result = base::Seconds(seconds);
    }
  }
  return result;
}

absl::optional<uint8_t> MaybeSetUint8FromCommandLine(base::CommandLine* cmdline,
                                                     const char* switch_name) {
  if (cmdline->HasSwitch(switch_name)) {
    unsigned value;
    if (base::StringToUint(cmdline->GetSwitchValueASCII(switch_name), &value)) {
      return value;
    }
  }
  return absl::nullopt;
}

std::string MaybeOverrideStringFromCommandLine(
    base::CommandLine* cmdline,
    const char* switch_name,
    std::string default_config_value) {
  std::string result = std::move(default_config_value);
  if (cmdline->HasSwitch(switch_name)) {
    result = cmdline->GetSwitchValueASCII(switch_name);
  }
  return result;
}

GURL MaybeOverrideURLFromCommandLine(base::CommandLine* cmdline,
                                     const char* switch_name,
                                     GURL default_config_value) {
  GURL result = std::move(default_config_value);
  if (cmdline->HasSwitch(switch_name)) {
    GURL url = GURL(cmdline->GetSwitchValueASCII(switch_name));
    if (url.is_valid()) {
      result = url;
    }
  }
  return result;
}

bool MaybeOverrideBoolFromCommandLine(base::CommandLine* cmdline,
                                      const char* switch_name,
                                      bool default_config_value) {
  bool result = default_config_value;
  if (cmdline->HasSwitch(switch_name)) {
    result = true;
  }
  return result;
}

inline void CheckURL(const GURL& url) {
#if defined(OFFICIAL_BUILD)
  CHECK(url.is_valid() && url.SchemeIsHTTPOrHTTPS());
#endif  // !OFFICIAL_BUILD
}

}  // namespace

P3AConfig::P3AConfig()
    : average_upload_interval(base::Seconds(kDefaultUploadIntervalSeconds)),
      randomize_upload_interval(true),
      p3a_json_upload_url(BUILDFLAG(P3A_JSON_UPLOAD_URL)),
      p3a_creative_upload_url(BUILDFLAG(P3A_CREATIVE_UPLOAD_URL)),
      p2a_json_upload_url(BUILDFLAG(P2A_JSON_UPLOAD_URL)),
      p3a_constellation_upload_host(BUILDFLAG(P3A_CONSTELLATION_UPLOAD_HOST)),
      star_randomness_host(BUILDFLAG(STAR_RANDOMNESS_HOST)) {
  CheckURL(p3a_json_upload_url);
  CheckURL(p3a_creative_upload_url);
  CheckURL(p2a_json_upload_url);
  CheckURL(GURL(star_randomness_host));
  for (MetricLogType log_type : kAllMetricLogTypes) {
    fake_star_epochs[log_type] = absl::nullopt;
  }
}

P3AConfig::~P3AConfig() = default;

P3AConfig::P3AConfig(const P3AConfig& config) = default;

P3AConfig P3AConfig::LoadFromCommandLine() {
  P3AConfig config;
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  config.average_upload_interval = MaybeOverrideTimeDeltaFromCommandLine(
      cmdline, switches::kP3AUploadIntervalSeconds,
      std::move(config.average_upload_interval));

  config.randomize_upload_interval =
      !cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval);

  config.json_rotation_intervals[MetricLogType::kSlow] =
      MaybeOverrideTimeDeltaFromCommandLine(
          cmdline, switches::kP3ASlowRotationIntervalSeconds,
          std::move(config.json_rotation_intervals[MetricLogType::kSlow]));
  config.json_rotation_intervals[MetricLogType::kTypical] =
      MaybeOverrideTimeDeltaFromCommandLine(
          cmdline, switches::kP3ATypicalRotationIntervalSeconds,
          std::move(config.json_rotation_intervals[MetricLogType::kTypical]));
  config.json_rotation_intervals[MetricLogType::kExpress] =
      MaybeOverrideTimeDeltaFromCommandLine(
          cmdline, switches::kP3AExpressRotationIntervalSeconds,
          std::move(config.json_rotation_intervals[MetricLogType::kExpress]));

  config.fake_star_epochs[MetricLogType::kSlow] =
      MaybeSetUint8FromCommandLine(cmdline, switches::kP3AFakeSlowStarEpoch);
  config.fake_star_epochs[MetricLogType::kTypical] =
      MaybeSetUint8FromCommandLine(cmdline, switches::kP3AFakeTypicalStarEpoch);
  config.fake_star_epochs[MetricLogType::kExpress] =
      MaybeSetUint8FromCommandLine(cmdline, switches::kP3AFakeExpressStarEpoch);

  config.p3a_json_upload_url =
      MaybeOverrideURLFromCommandLine(cmdline, switches::kP3AJsonUploadUrl,
                                      std::move(config.p3a_json_upload_url));
  config.p3a_creative_upload_url = MaybeOverrideURLFromCommandLine(
      cmdline, switches::kP3ACreativeUploadUrl,
      std::move(config.p3a_creative_upload_url));
  config.p2a_json_upload_url =
      MaybeOverrideURLFromCommandLine(cmdline, switches::kP2AJsonUploadUrl,
                                      std::move(config.p2a_json_upload_url));
  config.p3a_constellation_upload_host = MaybeOverrideStringFromCommandLine(
      cmdline, switches::kP3AConstellationUploadHost,
      std::move(config.p3a_constellation_upload_host));
  config.star_randomness_host = MaybeOverrideStringFromCommandLine(
      cmdline, switches::kP3AStarRandomnessHost,
      std::move(config.star_randomness_host));

  config.disable_star_attestation = MaybeOverrideBoolFromCommandLine(
      cmdline, switches::kP3ADisableStarAttestation,
      config.disable_star_attestation);

  config.ignore_server_errors = MaybeOverrideBoolFromCommandLine(
      cmdline, switches::kP3AIgnoreServerErrors, config.ignore_server_errors);

  VLOG(2) << "P3AConfig parameters are:"
          << ", average_upload_interval_ = " << config.average_upload_interval
          << ", randomize_upload_interval_ = "
          << config.randomize_upload_interval
          << ", p3a_json_upload_url_ = " << config.p3a_json_upload_url.spec()
          << ", p2a_json_upload_url_ = " << config.p2a_json_upload_url.spec()
          << ", p3a_creative_upload_url_ = "
          << config.p3a_creative_upload_url.spec()
          << ", p3a_constellation_upload_host_ = "
          << config.p3a_constellation_upload_host
          << ", star_randomness_host_ = " << config.star_randomness_host
          << ", ignore_server_errors_ = " << config.ignore_server_errors
          << ", disable_star_attestation = " << config.disable_star_attestation;
  return config;
}

}  // namespace p3a
