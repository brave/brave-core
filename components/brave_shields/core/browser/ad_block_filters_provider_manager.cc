// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/location.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"

namespace brave_shields {

AdBlockFiltersProviderManager::AdBlockFiltersProviderManager(
    bool suppress_initial_notification) {
  suppress_default_initial_ = suppress_initial_notification;
  suppress_additional_initial_ = suppress_initial_notification;
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

void AdBlockFiltersProviderManager::ForceNotifyObserver(
    AdBlockFiltersProvider::Observer& observer,
    bool is_for_default_engine) {
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

void AdBlockFiltersProviderManager::OnComponentProvidersRegistered() {
  component_providers_registered_ = true;
  // cache the list of components when registration finishes so we can wait for
  // them to initialize
  initial_default_engine_filters_providers_ = default_engine_filters_providers_;
  initial_additional_engine_filters_providers_ =
      additional_engine_filters_providers_;
}

void AdBlockFiltersProviderManager::OnChanged(bool is_for_default_engine) {
  // When a cached DAT is providing initial rules, suppress all notifications
  // until component providers have registered and the first "all providers
  // ready" event has been consumed.
  if (component_providers_registered_) {
    if (std::ranges::any_of(initial_default_engine_filters_providers_,
                            &AdBlockFiltersProvider::IsInitialized)) {
      suppress_default_initial_ = false;
      initial_default_engine_filters_providers_.clear();
    }
    if (std::ranges::any_of(initial_additional_engine_filters_providers_,
                            &AdBlockFiltersProvider::IsInitialized)) {
      suppress_additional_initial_ = false;
      initial_additional_engine_filters_providers_.clear();
    }
  }

  if (!AreAllProvidersInitialized(is_for_default_engine)) {
    return;
  }

  NotifyObservers(is_for_default_engine);
}

bool AdBlockFiltersProviderManager::AreAllProvidersInitialized(
    bool is_for_default_engine) const {
  bool suppress = is_for_default_engine ? suppress_default_initial_
                                        : suppress_additional_initial_;

  if (suppress) {
    // Still waiting for component providers — don't consume the flag yet.
    return false;
  }

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
