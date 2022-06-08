/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_message_manager_config.h"

#include <string>

#include "base/command_line.h"
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

MessageManagerConfig::MessageManagerConfig()
    : average_upload_interval(base::Seconds(kDefaultUploadIntervalSeconds)),
      randomize_upload_interval(true),
      p3a_upload_server_url(BUILDFLAG(P3A_JSON_SERVER_URL)),
      p2a_upload_server_url(BUILDFLAG(P2A_JSON_SERVER_URL)),
      star_randomness_url(BUILDFLAG(STAR_RANDOMNESS_SERVER_URL)) {}

void MessageManagerConfig::LoadFromCommandLine() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  LoadTimeDelta(cmdline, switches::kP3AUploadIntervalSeconds,
                &average_upload_interval);

  if (cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval)) {
    randomize_upload_interval = false;
  }

  LoadTimeDelta(cmdline, switches::kP3ARotationIntervalSeconds,
                &rotation_interval);

  LoadURL(cmdline, switches::kP3AUploadServerUrl, &p3a_upload_server_url);
  LoadURL(cmdline, switches::kP2AUploadServerUrl, &p2a_upload_server_url);
  LoadURL(cmdline, switches::kP3AStarRandomnessUrl, &star_randomness_url);

  LoadBool(cmdline, switches::kP3AIgnoreServerErrors, &ignore_server_errors);
  LoadBool(cmdline, switches::kP3AUseLocalRandomness, &use_local_randomness);
}

}  // namespace brave
