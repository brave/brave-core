/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_filters_provider_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/barrier_closure.h"
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
  NotifyObservers();
}

std::string AdBlockFiltersProviderManager::GetNameForDebugging() {
  return "AdBlockFiltersProviderManager";
}

void AdBlockFiltersProviderManager::OnChanged() {
  NotifyObservers();
}

// Use LoadDATBufferForEngine instead, for Filter Provider Manager.
void AdBlockFiltersProviderManager::LoadFilterSet(
    rust::Box<adblock::FilterSet>* filter_set,
    base::OnceCallback<void()>) {
  NOTREACHED();
}

void AdBlockFiltersProviderManager::LoadFilterSetForEngine(
    bool is_for_default_engine,
    rust::Box<adblock::FilterSet>* filter_set,
    base::OnceCallback<void()> cb) {
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  const auto collect_and_merge =
      base::BarrierClosure(filters_providers.size(), std::move(cb));
  for (auto* const provider : filters_providers) {
    task_tracker_.PostTask(
        base::SequencedTaskRunner::GetCurrentDefault().get(), FROM_HERE,
        base::BindOnce(&AdBlockFiltersProvider::LoadFilterSet,
                       provider->AsWeakPtr(), filter_set,
                       std::move(collect_and_merge)));
  }
}

}  // namespace brave_shields
