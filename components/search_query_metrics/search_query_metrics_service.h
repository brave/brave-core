// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_H_

#include "brave/components/search_query_metrics/search_query_metrics_entry_point_type.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;

namespace metrics {

class SearchQueryMetricsService : public KeyedService {
 public:
  SearchQueryMetricsService();

  SearchQueryMetricsService(const SearchQueryMetricsService&) = delete;
  SearchQueryMetricsService& operator=(const SearchQueryMetricsService&) =
      delete;

  ~SearchQueryMetricsService() override;

  virtual void MaybeReport(
      const GURL& url,
      SearchQueryMetricsEntryPointType entry_point_type) = 0;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_H_
