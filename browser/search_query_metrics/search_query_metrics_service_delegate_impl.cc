// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/search_query_metrics/search_query_metrics_service_delegate_impl.h"

#include "brave/browser/search_query_metrics/search_query_metrics_utils.h"
#include "brave/common/brave_channel_info.h"

namespace metrics {

std::string SearchQueryMetricsServiceDelegateImpl::GetBuildChannelName() const {
  return brave::GetChannelName();
}

bool SearchQueryMetricsServiceDelegateImpl::IsDefaultBrowser() const {
  return utils::IsDefaultBrowser();
}

}  // namespace metrics
