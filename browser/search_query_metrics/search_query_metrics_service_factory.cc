// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/search_query_metrics/search_query_metrics_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/search_query_metrics/search_query_metrics_service_delegate_impl.h"
#include "brave/components/search_query_metrics/network_client/network_client.h"
#include "brave/components/search_query_metrics/search_query_metrics_environment_util.h"
#include "brave/components/search_query_metrics/search_query_metrics_feature.h"
#include "brave/components/search_query_metrics/search_query_metrics_service.h"
#include "brave/components/search_query_metrics/search_query_metrics_service_impl.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace metrics {

namespace {

network::mojom::NetworkContext* GetNetworkContextForProfile(
    content::BrowserContext* context) {
  // Retrieves the `NetworkContext` from the default storage partition on
  // demand. A cached raw pointer will become invalid if the network service
  // crashes or restarts, so callers must not persist the returned pointer
  // beyond the current task.
  return context->GetDefaultStoragePartition()->GetNetworkContext();
}

}  // namespace

// static
SearchQueryMetricsService* SearchQueryMetricsServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SearchQueryMetricsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SearchQueryMetricsServiceFactory*
SearchQueryMetricsServiceFactory::GetInstance() {
  static base::NoDestructor<SearchQueryMetricsServiceFactory> instance;
  return instance.get();
}

SearchQueryMetricsServiceFactory::SearchQueryMetricsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SearchQueryMetricsService",
          BrowserContextDependencyManager::GetInstance()) {}

SearchQueryMetricsServiceFactory::~SearchQueryMetricsServiceFactory() = default;

content::BrowserContext*
SearchQueryMetricsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(kSearchQueryMetricsFeature)) {
    // Feature is disabled.
    return nullptr;
  }

  const Profile* const profile = Profile::FromBrowserContext(context);
  if (!kShouldReportForNonRegularProfile.Get() &&
      !profile->IsRegularProfile()) {
    // Reporting is disabled for non-regular profiles, unless overridden.
    return nullptr;
  }

  return context;
}

std::unique_ptr<KeyedService>
SearchQueryMetricsServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* const profile = Profile::FromBrowserContext(context);

  PrefService* const prefs = profile->GetPrefs();
  PrefService* const local_state = g_browser_process->local_state();

  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);

  content::StoragePartition* const default_store_partition =
      profile->GetDefaultStoragePartition();
  auto network_client = std::make_unique<NetworkClient>(
      *local_state,
      default_store_partition->GetURLLoaderFactoryForBrowserProcess(),
      base::BindRepeating(&GetNetworkContextForProfile, context),
      ShouldUseStagingEnvironment());

  auto delegate = std::make_unique<SearchQueryMetricsServiceDelegateImpl>();

  return std::make_unique<SearchQueryMetricsServiceImpl>(
      *prefs, *local_state, template_url_service, std::move(network_client),
      std::move(delegate));
}

}  // namespace metrics
