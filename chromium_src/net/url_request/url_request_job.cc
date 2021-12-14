/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_job.h"

// Strip referrer for cross-origin requests from a .onion hostname.
// This also affects the Origin header outside of CORS requests.
#define ComputeReferrerForPolicy                                              \
  ComputeReferrerForPolicy(                                                   \
      ReferrerPolicy policy, const GURL& original_referrer,                   \
      const GURL& destination, bool* same_origin_out_for_metrics) {           \
    if (base::EndsWith(original_referrer.host_piece(), ".onion",              \
                       base::CompareCase::INSENSITIVE_ASCII) &&               \
        !url::IsSameOriginWith(original_referrer, destination)) {             \
      return GURL();                                                          \
    }                                                                         \
    return ComputeReferrerForPolicy_Chromium(                                 \
        policy, original_referrer, destination, same_origin_out_for_metrics); \
  }                                                                           \
  GURL URLRequestJob::ComputeReferrerForPolicy_Chromium

#include "src/net/url_request/url_request_job.cc"

#undef ComputeReferrerForPolicy
