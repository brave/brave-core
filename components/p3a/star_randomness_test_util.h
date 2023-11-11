/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_TEST_UTIL_H_

#include <string>

#include "brave/components/p3a/metric_log_type.h"

class GURL;

namespace network {
struct ResourceRequest;
}  // namespace network

namespace p3a {

MetricLogType ValidateURLAndGetMetricLogType(const GURL& url,
                                             const char* expected_host);
std::string HandleRandomnessRequest(const network::ResourceRequest& request,
                                    uint8_t expected_epoch);
std::string HandleInfoRequest(const network::ResourceRequest& request,
                              MetricLogType log_type,
                              uint8_t current_epoch,
                              const char* next_epoch_time);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_TEST_UTIL_H_
