/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_config.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "brave/components/p3a/brave_p3a_switches.h"
#include "brave/components/p3a/buildflags.h"

namespace brave {

namespace {

constexpr uint64_t kDefaultUploadIntervalSeconds = 60;  // 1 minute.

void LoadTimeDelta(base::CommandLine* cmdline,
                   const char* switch_name,
                   base::TimeDelta* result) {
  if (cmdline->HasSwitch(switch_name)) {
    std::string seconds_str = cmdline->GetSwitchValueASCII(switch_name);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      *result = base::Seconds(seconds);
    }
  }
}

void LoadString(base::CommandLine* cmdline,
                const char* switch_name,
                std::string* result) {
  if (cmdline->HasSwitch(switch_name)) {
    *result = cmdline->GetSwitchValueASCII(switch_name);
  }
}

void LoadURL(base::CommandLine* cmdline,
             const char* switch_name,
             GURL* result) {
  if (cmdline->HasSwitch(switch_name)) {
    GURL url = GURL(cmdline->GetSwitchValueASCII(switch_name));
    if (url.is_valid()) {
      *result = url;
    }
  }
}

void LoadBool(base::CommandLine* cmdline,
              const char* switch_name,
              bool* result) {
  if (cmdline->HasSwitch(switch_name)) {
    *result = true;
  }
}

}  // namespace

BraveP3AConfig::BraveP3AConfig()
    : average_upload_interval(base::Seconds(kDefaultUploadIntervalSeconds)),
      randomize_upload_interval(true),
      p3a_json_upload_url(BUILDFLAG(P3A_JSON_UPLOAD_URL)),
      p2a_json_upload_url(BUILDFLAG(P2A_JSON_UPLOAD_URL)),
      p3a_star_upload_url(BUILDFLAG(P3A_STAR_UPLOAD_URL)),
      p2a_star_upload_url(BUILDFLAG(P2A_STAR_UPLOAD_URL)),
      star_randomness_host(BUILDFLAG(STAR_RANDOMNESS_HOST)) {}

BraveP3AConfig::~BraveP3AConfig() {}

void BraveP3AConfig::LoadFromCommandLine() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  LoadTimeDelta(cmdline, switches::kP3AUploadIntervalSeconds,
                &average_upload_interval);

  if (cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval)) {
    randomize_upload_interval = false;
  }

  LoadTimeDelta(cmdline, switches::kP3ARotationIntervalSeconds,
                &rotation_interval);

  LoadURL(cmdline, switches::kP3AJsonUploadUrl, &p3a_json_upload_url);
  LoadURL(cmdline, switches::kP2AJsonUploadUrl, &p2a_json_upload_url);
  LoadURL(cmdline, switches::kP3AStarUploadUrl, &p3a_star_upload_url);
  LoadURL(cmdline, switches::kP2AStarUploadUrl, &p2a_star_upload_url);
  LoadString(cmdline, switches::kP3AStarRandomnessHost, &star_randomness_host);

  LoadBool(cmdline, switches::kP3ADisableStarAttestation,
           &disable_star_attestation);

  LoadBool(cmdline, switches::kP3AIgnoreServerErrors, &ignore_server_errors);

  VLOG(2) << "BraveP3AConfig parameters are:"
          << ", average_upload_interval = " << average_upload_interval
          << ", randomize_upload_interval = " << randomize_upload_interval
          << ", p3a_json_upload_url = " << p3a_json_upload_url.spec()
          << ", p2a_json_upload_url = " << p2a_json_upload_url.spec()
          << ", p3a_star_upload_url = " << p3a_star_upload_url.spec()
          << ", p2a_star_upload_url = " << p2a_star_upload_url.spec()
          << ", star_randomness_host = " << star_randomness_host
          << ", rotation_interval = " << rotation_interval
          << ", disable_star_attestation = " << disable_star_attestation;
}

}  // namespace brave
