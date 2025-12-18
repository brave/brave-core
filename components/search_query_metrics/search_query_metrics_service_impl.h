// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_IMPL_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/search_query_metrics/network_client/network_client.h"
#include "brave/components/search_query_metrics/search_query_metrics_entry_point_type.h"
#include "brave/components/search_query_metrics/search_query_metrics_service.h"
#include "brave/components/search_query_metrics/search_query_metrics_service_delegate.h"
#include "components/search_engines/template_url_service.h"

class GURL;
class PrefService;

namespace metrics {

struct QueueItemInfo;

// Builds the payload and manages search query metric reporting by queuing
// reports in an ephemeral in-memory queue, sending them to the endpoint, and
// retrying on failure.

class SearchQueryMetricsServiceImpl : public SearchQueryMetricsService {
 public:
  explicit SearchQueryMetricsServiceImpl(
      PrefService& prefs,
      PrefService& local_state,
      TemplateURLService* template_url_service,
      std::unique_ptr<NetworkClient> network_client,
      std::unique_ptr<SearchQueryMetricsServiceDelegate> delegate);

  SearchQueryMetricsServiceImpl(const SearchQueryMetricsServiceImpl&) = delete;
  SearchQueryMetricsServiceImpl& operator=(
      const SearchQueryMetricsServiceImpl&) = delete;

  ~SearchQueryMetricsServiceImpl() override;

  // SearchQueryMetricsService:
  void MaybeReport(const GURL& url,
                   SearchQueryMetricsEntryPointType entry_point_type) override;

  // KeyedService:
  void Shutdown() override;

 private:
  void QueueReport(const std::string& payload);
  void Report(QueueItemInfo queue_item);
  void ReportCallback(
      QueueItemInfo queue_item,
      const GURL& url,
      int response_code,
      const std::string& response_body,
      const base::flat_map<std::string, std::string>& response_headers);
  void MaybeRetry(QueueItemInfo queue_item);

  // See `README.md` for payload structure.
  std::string BuildPayload(
      const GURL& url,
      SearchQueryMetricsEntryPointType entry_point_type) const;
  std::optional<std::string> BuildChannel() const;
  std::optional<std::string> Country() const;
  std::optional<std::string> DefaultSearchEngine() const;
  bool IsDefaultBrowser() const;
  bool IsFirstQuery() const;

  const raw_ref<PrefService> prefs_;        // Not owned.
  const raw_ref<PrefService> local_state_;  // Not owned.

  const raw_ptr<TemplateURLService> template_url_service_;  // Not owned.

  const std::unique_ptr<NetworkClient> network_client_;

  const std::unique_ptr<SearchQueryMetricsServiceDelegate> delegate_;

  base::WeakPtrFactory<SearchQueryMetricsServiceImpl> weak_ptr_factory_{this};
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_IMPL_H_
