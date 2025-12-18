// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_DELEGATE_IMPL_H_

#include <string>

#include "brave/components/search_query_metrics/search_query_metrics_service_delegate.h"

namespace metrics {

class SearchQueryMetricsServiceDelegateImpl
    : public SearchQueryMetricsServiceDelegate {
 public:
  SearchQueryMetricsServiceDelegateImpl() = default;
  ~SearchQueryMetricsServiceDelegateImpl() override = default;

  std::string GetBuildChannelName() const override;

  bool IsDefaultBrowser() const override;
};
}  // namespace metrics

#endif  // BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_DELEGATE_IMPL_H_
