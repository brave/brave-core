/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_JOB_H_
#define BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_JOB_H_

#define ComputeReferrerForPolicy                                             \
  ComputeReferrerForPolicy(                                                  \
      ReferrerPolicy policy, const GURL& original_referrer,                  \
      const GURL& destination, bool* same_origin_out_for_metrics = nullptr); \
  static GURL ComputeReferrerForPolicy_Chromium

#include "src/net/url_request/url_request_job.h"

#undef ComputeReferrerForPolicy

#endif  // BRAVE_CHROMIUM_SRC_NET_URL_REQUEST_URL_REQUEST_JOB_H_
