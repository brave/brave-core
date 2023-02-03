/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_CONFIG_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_CONFIG_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "brave/components/p3a/metric_log_type.h"
#include "url/gurl.h"

namespace brave {

struct BraveP3AConfig {
  // The average interval between uploading different values.
  base::TimeDelta average_upload_interval;
  bool randomize_upload_interval = true;
  // Interval between rotations, only used for testing from the command line.
  base::flat_map<MetricLogType, base::TimeDelta> json_rotation_intervals;

  // Endpoint for uploading P3A metrics in JSON format
  GURL p3a_json_upload_url;
  // Endpoint for uploading NTP-SI/creative P3A metrics in JSON format
  GURL p3a_creative_upload_url;
  // Endpoint for uploading P2A metrics in JSON format
  GURL p2a_json_upload_url;
  // Endpoint for uploading P3A metrics encrypted by Constellation/STAR
  GURL p3a_star_upload_url;
  // Host for generating randomness points for STAR encryption of measurements
  std::string star_randomness_host;

  // Disable Nitro Enclave attestation of the randomness server
  bool disable_star_attestation = false;

  bool ignore_server_errors = false;

  BraveP3AConfig();
  ~BraveP3AConfig();
  BraveP3AConfig(const BraveP3AConfig& config);

  static BraveP3AConfig LoadFromCommandLine();
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_CONFIG_H_
