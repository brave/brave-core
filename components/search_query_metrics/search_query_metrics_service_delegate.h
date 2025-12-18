// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_DELEGATE_H_

#include <string>

namespace metrics {

class SearchQueryMetricsServiceDelegate {
 public:
  virtual ~SearchQueryMetricsServiceDelegate() = default;

  virtual std::string GetBuildChannelName() const = 0;

  virtual bool IsDefaultBrowser() const = 0;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_DELEGATE_H_
