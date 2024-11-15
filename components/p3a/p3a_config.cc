/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_config.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/switches.h"
#include "url/url_constants.h"

namespace p3a {

namespace {

constexpr uint64_t kDefaultUploadIntervalSeconds = 60;  // 1 minute.
constexpr char kP3AJsonHostPrefix[] = "p3a-json";
constexpr char kP3ACreativeHostPrefix[] = "p3a-creative";
constexpr char kP2AJsonHostPrefix[] = "p2a-json";
constexpr char kJsonURLPath[] = "/";
constexpr char kConstellationCollectorHostPrefix[] = "collector.bsg";
constexpr char kRandomnessHostPrefix[] = "star-randsrv.bsg";

base::TimeDelta MaybeOverrideTimeDeltaFromCommandLine(
    base::CommandLine* cmdline,
    const char* switch_name,
    base::TimeDelta default_config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    std::string seconds_str = cmdline->GetSwitchValueASCII(switch_name);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      return base::Seconds(seconds);
    }
  }
  return default_config_value;
}

std::optional<uint8_t> MaybeSetUint8FromCommandLine(base::CommandLine* cmdline,
                                                    const char* switch_name) {
  if (cmdline->HasSwitch(switch_name)) {
    unsigned value;
    if (base::StringToUint(cmdline->GetSwitchValueASCII(switch_name), &value)) {
      return value;
    }
  }
  return std::nullopt;
}

std::string MaybeOverrideStringFromCommandLine(
    base::CommandLine* cmdline,
    const char* switch_name,
    std::string default_config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    return cmdline->GetSwitchValueASCII(switch_name);
  }
  return default_config_value;
}

GURL MaybeOverrideURLFromCommandLine(base::CommandLine* cmdline,
                                     const char* switch_name,
                                     GURL default_config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    GURL url = GURL(cmdline->GetSwitchValueASCII(switch_name));
    if (url.is_valid()) {
      return url;
    }
  }
  return default_config_value;
}

bool MaybeOverrideBoolFromCommandLine(base::CommandLine* cmdline,
                                      const char* switch_name,
                                      bool default_config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    return true;
  }
  return default_config_value;
}

inline void CheckURL(const GURL& url) {
#if defined(OFFICIAL_BUILD)
  CHECK(url.is_valid() && url.SchemeIsHTTPOrHTTPS());
#endif  // !OFFICIAL_BUILD
}

std::string GetDefaultHost(const char* host_prefix) {
  return base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                       brave_domains::GetServicesDomain(host_prefix)});
}

GURL GetDefaultURL(const char* host_prefix, const char* path) {
  return GURL(base::StrCat({GetDefaultHost(host_prefix), path}));
}

}  // namespace

P3AConfig::P3AConfig()
    : average_upload_interval(base::Seconds(kDefaultUploadIntervalSeconds)),
      randomize_upload_interval(true),
      p3a_json_upload_url(GetDefaultURL(kP3AJsonHostPrefix, kJsonURLPath)),
      p3a_creative_upload_url(
          GetDefaultURL(kP3ACreativeHostPrefix, kJsonURLPath)),
      p2a_json_upload_url(GetDefaultURL(kP2AJsonHostPrefix, kJsonURLPath)),
      p3a_constellation_upload_host(
          GetDefaultHost(kConstellationCollectorHostPrefix)),
      star_randomness_host(GetDefaultHost(kRandomnessHostPrefix)) {
  CheckURL(p3a_json_upload_url);
  CheckURL(p3a_creative_upload_url);
  CheckURL(p2a_json_upload_url);
  CheckURL(GURL(star_randomness_host));
  for (MetricLogType log_type : kAllMetricLogTypes) {
    fake_star_epochs[log_type] = std::nullopt;
  }
}

P3AConfig::~P3AConfig() = default;

P3AConfig::P3AConfig(const P3AConfig& config) = default;

P3AConfig P3AConfig::LoadFromCommandLine() {
  P3AConfig config;
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  config.average_upload_interval = MaybeOverrideTimeDeltaFromCommandLine(
      cmdline, switches::kP3AUploadIntervalSeconds,
      config.average_upload_interval);

  config.randomize_upload_interval =
      !cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval);

  config.json_rotation_intervals[MetricLogType::kSlow] =
      MaybeOverrideTimeDeltaFromCommandLine(
          cmdline, switches::kP3ASlowRotationIntervalSeconds,
          config.json_rotation_intervals[MetricLogType::kSlow]);
  config.json_rotation_intervals[MetricLogType::kTypical] =
      MaybeOverrideTimeDeltaFromCommandLine(
          cmdline, switches::kP3ATypicalRotationIntervalSeconds,
          config.json_rotation_intervals[MetricLogType::kTypical]);
  config.json_rotation_intervals[MetricLogType::kExpress] =
      MaybeOverrideTimeDeltaFromCommandLine(
          cmdline, switches::kP3AExpressRotationIntervalSeconds,
          config.json_rotation_intervals[MetricLogType::kExpress]);

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
