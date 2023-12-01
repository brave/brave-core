/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_P3A_CONFIG_H_
#define BRAVE_COMPONENTS_P3A_P3A_CONFIG_H_

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "brave/components/p3a/metric_log_type.h"
#include "url/gurl.h"

namespace p3a {

struct P3AConfig {
  // The average interval between uploading different values.
  base::TimeDelta average_upload_interval;
  bool randomize_upload_interval = true;
  // Interval between rotations, only used for testing from the command line.
  base::flat_map<MetricLogType, base::TimeDelta> json_rotation_intervals;

  // Fake STAR epoch for testing purposes.
  base::flat_map<MetricLogType, std::optional<uint8_t>> fake_star_epochs;

  // Endpoint for uploading P3A metrics in JSON format
  GURL p3a_json_upload_url;
  // Endpoint for uploading NTP-SI/creative P3A metrics in JSON format
  GURL p3a_creative_upload_url;
  // Endpoint for uploading P2A metrics in JSON format
  GURL p2a_json_upload_url;
  // Host for uploading P3A metrics encrypted by Constellation/STAR
  std::string p3a_constellation_upload_host;
  // Host for generating randomness points for STAR encryption of measurements
  std::string star_randomness_host;

  // Disable Nitro Enclave attestation of the randomness server
  bool disable_star_attestation = false;

  bool ignore_server_errors = false;

  P3AConfig();
  ~P3AConfig();
  P3AConfig(const P3AConfig& config);

  static P3AConfig LoadFromCommandLine();
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_P3A_CONFIG_H_
