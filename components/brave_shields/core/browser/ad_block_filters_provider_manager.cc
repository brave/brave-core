// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_shields {

// static
AdBlockFiltersProviderManager* AdBlockFiltersProviderManager::GetInstance() {
  static base::NoDestructor<AdBlockFiltersProviderManager> instance;
  return instance.get();
}

AdBlockFiltersProviderManager::AdBlockFiltersProviderManager() = default;

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

std::string AdBlockFiltersProviderManager::GetNameForDebugging() {
  return "AdBlockFiltersProviderManager";
}

void AdBlockFiltersProviderManager::OnChanged(bool is_for_default_engine) {
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  for (auto*& provider : filters_providers) {
    if (!provider->IsInitialized()) {
      return;
    }
  }
  NotifyObservers(is_for_default_engine);
}

// Use LoadDATBufferForEngine instead, for Filter Provider Manager.
void AdBlockFiltersProviderManager::LoadFilterSet(
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)>) {
  NOTREACHED_IN_MIGRATION();
}

void AdBlockFiltersProviderManager::LoadFilterSetForEngine(
    bool is_for_default_engine,
    base::OnceCallback<
        void(base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) {
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  auto collect_and_merge = base::BarrierCallback<
      base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>>(
      filters_providers.size(),
      base::BindOnce(&AdBlockFiltersProviderManager::FinishCombinating,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
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
    std::vector<base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>>
        results) {
  std::move(cb).Run(base::BindOnce(&RunAllResults, std::move(results)));
}

}  // namespace brave_shields
