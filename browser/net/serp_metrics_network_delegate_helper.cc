/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/serp_metrics_network_delegate_helper.h"

#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/serp_metrics/ai_serp_classifier_util.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "net/base/net_errors.h"

namespace {

serp_metrics::SerpMetrics* GetSerpMetrics(
    content::BrowserContext* browser_context) {
  if (!browser_context || browser_context->IsOffTheRecord()) {
    return nullptr;
  }

  misc_metrics::ProfileMiscMetricsService* profile_misc_metrics_service =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
          browser_context);
  if (!profile_misc_metrics_service) {
    return nullptr;
  }

  return profile_misc_metrics_service->GetSerpMetrics();
}

}  // namespace

namespace brave {

template <template <typename> class T>
int OnBeforeStartTransaction_SerpMetricsWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    T<BraveRequestInfo> request) {
  CHECK(request);
  serp_metrics::SerpMetrics* serp_metrics =
      GetSerpMetrics(request->browser_context());
  if (!serp_metrics) {
    return net::OK;
  }

  std::optional<serp_metrics::SerpMetricType> serp_metric_type =
      serp_metrics::MaybeClassifyAISerp(request->request_url());
  if (serp_metric_type) {
    // TODO: Consider doing RecordSearch asynchronously by posting a task with
    // RecordSearch to the UI thread.
    serp_metrics->RecordSearch(*serp_metric_type);
  }
  return net::OK;
}

template int OnBeforeStartTransaction_SerpMetricsWork<std::shared_ptr>(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> request);

template int OnBeforeStartTransaction_SerpMetricsWork<base::WeakPtr>(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    base::WeakPtr<BraveRequestInfo> request);

}  // namespace brave
