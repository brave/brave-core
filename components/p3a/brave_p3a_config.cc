/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_config.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "brave/components/p3a/brave_p3a_switches.h"
#include "brave/components/p3a/buildflags.h"

namespace brave {

namespace {

constexpr uint64_t kDefaultUploadIntervalSeconds = 60;  // 1 minute.

void MaybeReadTimeDeltaFromCommandLine(base::CommandLine* cmdline,
                                       const char* switch_name,
                                       base::TimeDelta* config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    std::string seconds_str = cmdline->GetSwitchValueASCII(switch_name);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      *config_value = base::Seconds(seconds);
    }
  }
}

void MaybeReadStringFromCommandLine(base::CommandLine* cmdline,
                                    const char* switch_name,
                                    std::string* config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    *config_value = cmdline->GetSwitchValueASCII(switch_name);
  }
}

void MaybeReadURLFromCommandLine(base::CommandLine* cmdline,
                                 const char* switch_name,
                                 GURL* config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    GURL url = GURL(cmdline->GetSwitchValueASCII(switch_name));
    if (url.is_valid()) {
      *config_value = url;
    }
  }
}

void MaybeReadBoolFromCommandLine(base::CommandLine* cmdline,
                                  const char* switch_name,
                                  bool* config_value) {
  if (cmdline->HasSwitch(switch_name)) {
    *config_value = true;
  }
}

}  // namespace

BraveP3AConfig::BraveP3AConfig()
    : average_upload_interval(base::Seconds(kDefaultUploadIntervalSeconds)),
      randomize_upload_interval(true),
      p3a_json_upload_url(BUILDFLAG(P3A_JSON_UPLOAD_URL)),
      p3a_creative_upload_url(BUILDFLAG(P3A_CREATIVE_UPLOAD_URL)),
      p2a_json_upload_url(BUILDFLAG(P2A_JSON_UPLOAD_URL)),
      p3a_star_upload_url(BUILDFLAG(P3A_STAR_UPLOAD_URL)),
      star_randomness_host(BUILDFLAG(STAR_RANDOMNESS_HOST)) {}

BraveP3AConfig::~BraveP3AConfig() = default;

BraveP3AConfig::BraveP3AConfig(const BraveP3AConfig& config) = default;

BraveP3AConfig BraveP3AConfig::LoadFromCommandLine() {
  BraveP3AConfig config;
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  MaybeReadTimeDeltaFromCommandLine(cmdline,
                                    switches::kP3AUploadIntervalSeconds,
                                    &config.average_upload_interval);

  config.randomize_upload_interval =
      !cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval);

  MaybeReadTimeDeltaFromCommandLine(
      cmdline, switches::kP3ASlowRotationIntervalSeconds,
      &config.json_rotation_intervals[MetricLogType::kSlow]);
  MaybeReadTimeDeltaFromCommandLine(
      cmdline, switches::kP3ATypicalRotationIntervalSeconds,
      &config.json_rotation_intervals[MetricLogType::kTypical]);
  MaybeReadTimeDeltaFromCommandLine(
      cmdline, switches::kP3AExpressRotationIntervalSeconds,
      &config.json_rotation_intervals[MetricLogType::kExpress]);

  MaybeReadURLFromCommandLine(cmdline, switches::kP3AJsonUploadUrl,
                              &config.p3a_json_upload_url);
  MaybeReadURLFromCommandLine(cmdline, switches::kP3ACreativeUploadUrl,
                              &config.p3a_creative_upload_url);
  MaybeReadURLFromCommandLine(cmdline, switches::kP2AJsonUploadUrl,
                              &config.p2a_json_upload_url);
  MaybeReadURLFromCommandLine(cmdline, switches::kP3AStarUploadUrl,
                              &config.p3a_star_upload_url);
  MaybeReadStringFromCommandLine(cmdline, switches::kP3AStarRandomnessHost,
                                 &config.star_randomness_host);

  MaybeReadBoolFromCommandLine(cmdline, switches::kP3ADisableStarAttestation,
                               &config.disable_star_attestation);

  MaybeReadBoolFromCommandLine(cmdline, switches::kP3AIgnoreServerErrors,
                               &config.ignore_server_errors);

  VLOG(2) << "BraveP3AConfig parameters are:"
          << ", average_upload_interval_ = " << config.average_upload_interval
          << ", randomize_upload_interval_ = "
          << config.randomize_upload_interval
          << ", p3a_json_upload_url_ = " << config.p3a_json_upload_url.spec()
          << ", p2a_json_upload_url_ = " << config.p2a_json_upload_url.spec()
          << ", p3a_creative_upload_url_ = "
          << config.p3a_creative_upload_url.spec()
          << ", p3a_star_upload_url_ = " << config.p3a_star_upload_url.spec()
          << ", star_randomness_host_ = " << config.star_randomness_host
          << ", ignore_server_errors_ = " << config.ignore_server_errors
          << ", disable_star_attestation = " << config.disable_star_attestation;
  return config;
}

}  // namespace brave
