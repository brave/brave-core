// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/common/features.h"

namespace brave_shields {

AdBlockFiltersProviderManager::AdBlockFiltersProviderManager() {
  suppress_default_engine_startup_change_notification_ =
      base::FeatureList::IsEnabled(features::kAdblockDATCache);
  suppress_additional_engine_startup_change_notification_ =
      base::FeatureList::IsEnabled(features::kAdblockDATCache);
}

AdBlockFiltersProviderManager::~AdBlockFiltersProviderManager() = default;

void AdBlockFiltersProviderManager::AddProvider(
    AdBlockFiltersProvider* provider,
    bool is_for_default_engine) {
  auto rv = is_for_default_engine
                ? default_engine_filters_providers_.insert(provider)
                : additional_engine_filters_providers_.insert(provider);
  DCHECK(rv.second);
  provider->AddObserver(this);
}

void AdBlockFiltersProviderManager::RemoveProvider(
    AdBlockFiltersProvider* provider,
    bool is_for_default_engine) {
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  auto it = filters_providers.find(provider);
  DCHECK(it != filters_providers.end());
  filters_providers.erase(it);

  NotifyObservers(is_for_default_engine);
}

void AdBlockFiltersProviderManager::MaybeNotifyObserver(
    AdBlockFiltersProvider::Observer& observer,
    bool is_for_default_engine) {
  if (AreAllProvidersInitialized(is_for_default_engine)) {
    observer.OnChanged(is_for_default_engine);
  }
}

void AdBlockFiltersProviderManager::ForceNotifyObserver(
    AdBlockFiltersProvider::Observer& observer,
    bool is_for_default_engine) {
  // Consume the startup change notification if it hasn't been consumed already.
  // If it has been consumed this will return false and continue to OnChanged.
  // If it has not been consumed then we're still waiting for a startup provider
  // to initialize and it will notify when it does instead of suppressing it.
  if (MaybeConsumeEngineStartupChangeNotification(is_for_default_engine)) {
    return;
  }

  if (AreAllProvidersInitialized(is_for_default_engine)) {
    observer.OnChanged(is_for_default_engine);
  }
}

std::string AdBlockFiltersProviderManager::GetNameForDebugging() {
  return "AdBlockFiltersProviderManager";
}

const base::flat_set<AdBlockFiltersProvider*>&
AdBlockFiltersProviderManager::GetProviders(bool is_for_default_engine) const {
  return is_for_default_engine ? default_engine_filters_providers_
                               : additional_engine_filters_providers_;
}

void AdBlockFiltersProviderManager::OnChanged(bool is_for_default_engine) {
  if (!AreAllProvidersInitialized(is_for_default_engine)) {
    return;
  }

  // All startup providers are initialized so we can safely consume the startup
  // notification. The next call to OnChanged will trigger NotifyObservers
  if (MaybeConsumeEngineStartupChangeNotification(is_for_default_engine)) {
    return;
  }

  NotifyObservers(is_for_default_engine);
}

bool AdBlockFiltersProviderManager::MaybeConsumeEngineStartupChangeNotification(
    bool is_for_default_engine) {
  // In most cases we should be loading from DAT cache so we want to consume
  // the startup OnChanged notification to avoid parsing the filter list
  // We explicitly don't try to handle edge cases like updates that happen
  // during startup/shutdown and (at least for now) wait for the next component
  // update to clear it
  bool& suppress_engine_startup_change_notifications =
      is_for_default_engine
          ? suppress_default_engine_startup_change_notification_
          : suppress_additional_engine_startup_change_notification_;
  if (suppress_engine_startup_change_notifications) {
    suppress_engine_startup_change_notifications = false;
    return true;
  }
  return false;
}

bool AdBlockFiltersProviderManager::AreAllProvidersInitialized(
    bool is_for_default_engine) const {
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;

  if (filters_providers.empty()) {
    return false;
  }
  for (auto* const& provider : filters_providers) {
    if (!provider->IsInitialized()) {
      return false;
    }
  }
  return true;
}

// Use LoadDATBufferForEngine instead, for Filter Provider Manager.
void AdBlockFiltersProviderManager::LoadFilterSet(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)>) {
  NOTREACHED();
}

void AdBlockFiltersProviderManager::LoadFilterSetForEngine(
    bool is_for_default_engine,
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) {
  const uint64_t flow_id = base::RandUint64();
  TRACE_EVENT("brave.adblock",
              "AdBlockFiltersProviderManager::LoadFilterSetForEngine",
              perfetto::Flow::ProcessScoped(flow_id), "is_default_engine",
              is_for_default_engine);
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  auto collect_and_merge = base::BarrierCallback<
      base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>>(
      filters_providers.size(),
      base::BindOnce(&AdBlockFiltersProviderManager::FinishCombinating,
                     weak_factory_.GetWeakPtr(), std::move(cb), flow_id));
  for (auto* const provider : filters_providers) {
    task_tracker_.PostTask(
        base::SequencedTaskRunner::GetCurrentDefault().get(), FROM_HERE,
        base::BindOnce(&AdBlockFiltersProvider::LoadFilterSet,
                       provider->AsWeakPtr(), collect_and_merge));
  }
}

// static
void RunAllResults(
    std::vector<base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>>
        results,
    rust::Box<adblock::FilterSet>* filter_set) {
  for (auto& cb : results) {
    std::move(cb).Run(filter_set);
  }
}

void AdBlockFiltersProviderManager::FinishCombinating(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb,
    uint64_t flow_id,
    std::vector<base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>>
        results) {
  TRACE_EVENT("brave.adblock",
              "AdBlockFiltersProviderManager::FinishCombinating",
              perfetto::TerminatingFlow::ProcessScoped(flow_id));
  std::move(cb).Run(base::BindOnce(&RunAllResults, std::move(results)));
}

}  // namespace brave_shields
