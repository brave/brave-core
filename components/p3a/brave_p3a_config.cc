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

base::TimeDelta GetTimeDeltaFromCommandLine(base::CommandLine* cmdline,
                                            const char* switch_name) {
  if (cmdline->HasSwitch(switch_name)) {
    std::string seconds_str = cmdline->GetSwitchValueASCII(switch_name);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      return base::Seconds(seconds);
    }
  }
  return base::TimeDelta();
}

std::string GetStringFromCommandLine(base::CommandLine* cmdline,
                                     const char* switch_name) {
  if (cmdline->HasSwitch(switch_name)) {
    return cmdline->GetSwitchValueASCII(switch_name);
  }
  return std::string();
}

GURL GetURLFromCommandLine(base::CommandLine* cmdline,
                           const char* switch_name) {
  if (cmdline->HasSwitch(switch_name)) {
    GURL url = GURL(cmdline->GetSwitchValueASCII(switch_name));
#if defined(OFFICIAL_BUILD)
    CHECK(url.is_valid());
#endif
    if (url.is_valid()) {
      return url;
    }
  }
  return GURL();
}

bool GetBoolFromCommandLine(base::CommandLine* cmdline,
                            const char* switch_name) {
  if (cmdline->HasSwitch(switch_name)) {
    return true;
  }
  return false;
}

}  // namespace

BraveP3AConfig::BraveP3AConfig()
    : average_upload_interval(base::Seconds(kDefaultUploadIntervalSeconds)),
      randomize_upload_interval(true),
      p3a_json_upload_url(BUILDFLAG(P3A_JSON_UPLOAD_URL)),
      p3a_creative_upload_url(BUILDFLAG(P3A_CREATIVE_UPLOAD_URL)),
      p2a_json_upload_url(BUILDFLAG(P2A_JSON_UPLOAD_URL)),
      p3a_star_upload_url(BUILDFLAG(P3A_STAR_UPLOAD_URL)),
      p2a_star_upload_url(BUILDFLAG(P2A_STAR_UPLOAD_URL)),
      star_randomness_host(BUILDFLAG(STAR_RANDOMNESS_HOST)) {}

BraveP3AConfig::~BraveP3AConfig() {}

void BraveP3AConfig::LoadFromCommandLine() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  average_upload_interval =
      GetTimeDeltaFromCommandLine(cmdline, switches::kP3AUploadIntervalSeconds);

  if (cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval)) {
    randomize_upload_interval = false;
  }

  json_rotation_intervals[MetricLogType::kSlow] = GetTimeDeltaFromCommandLine(
      cmdline, switches::kP3ASlowRotationIntervalSeconds);
  json_rotation_intervals[MetricLogType::kTypical] =
      GetTimeDeltaFromCommandLine(cmdline,
                                  switches::kP3ATypicalRotationIntervalSeconds);
  json_rotation_intervals[MetricLogType::kExpress] =
      GetTimeDeltaFromCommandLine(cmdline,
                                  switches::kP3AExpressRotationIntervalSeconds);

  p3a_json_upload_url =
      GetURLFromCommandLine(cmdline, switches::kP3AJsonUploadUrl);
  p3a_creative_upload_url =
      GetURLFromCommandLine(cmdline, switches::kP3ACreativeUploadUrl);
  p2a_json_upload_url =
      GetURLFromCommandLine(cmdline, switches::kP2AJsonUploadUrl);
  p3a_star_upload_url =
      GetURLFromCommandLine(cmdline, switches::kP3AStarUploadUrl);
  p2a_star_upload_url =
      GetURLFromCommandLine(cmdline, switches::kP2AStarUploadUrl);
  star_randomness_host =
      GetStringFromCommandLine(cmdline, switches::kP3AStarRandomnessHost);

  disable_star_attestation =
      GetBoolFromCommandLine(cmdline, switches::kP3ADisableStarAttestation);

  ignore_server_errors =
      GetBoolFromCommandLine(cmdline, switches::kP3AIgnoreServerErrors);

  VLOG(2) << "BraveP3AConfig parameters are:"
          << ", average_upload_interval_ = " << average_upload_interval
          << ", randomize_upload_interval_ = " << randomize_upload_interval
          << ", p3a_json_upload_url_ = " << p3a_json_upload_url.spec()
          << ", p2a_json_upload_url_ = " << p2a_json_upload_url.spec()
          << ", p3a_creative_upload_url_ = " << p3a_creative_upload_url.spec()
          << ", p3a_star_upload_url_ = " << p3a_star_upload_url.spec()
          << ", p2a_star_upload_url_ = " << p2a_star_upload_url.spec()
          << ", star_randomness_host_ = " << star_randomness_host
          << ", disable_star_attestation = " << disable_star_attestation;
}

}  // namespace brave
