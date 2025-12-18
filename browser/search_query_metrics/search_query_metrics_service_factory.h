// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace metrics {

class SearchQueryMetricsService;

class SearchQueryMetricsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  SearchQueryMetricsServiceFactory(const SearchQueryMetricsServiceFactory&) =
      delete;
  SearchQueryMetricsServiceFactory& operator=(
      const SearchQueryMetricsServiceFactory&) = delete;

  static SearchQueryMetricsService* GetForProfile(Profile* profile);

  static SearchQueryMetricsServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<SearchQueryMetricsServiceFactory>;

  SearchQueryMetricsServiceFactory();

  ~SearchQueryMetricsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace metrics

#endif  // BRAVE_BROWSER_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_SERVICE_FACTORY_H_
