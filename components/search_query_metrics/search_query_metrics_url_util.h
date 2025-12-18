/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_URL_UTIL_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_URL_UTIL_H_

class GURL;

namespace metrics {

GURL GetUrl(bool use_staging);

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_URL_UTIL_H_
