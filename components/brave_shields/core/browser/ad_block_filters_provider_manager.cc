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
    base::OnceCallback<void(std::pair<uint8_t, DATFileDataBuffer>)>) {
  NOTREACHED();
}

void AdBlockFiltersProviderManager::GetFiltersForEngine(
    bool is_for_default_engine,
    base::OnceCallback<void(std::vector<std::pair<uint8_t, DATFileDataBuffer>>)>
        cb) {
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  const auto collect_and_merge =
      base::BarrierCallback<std::pair<uint8_t, DATFileDataBuffer>>(
          filters_providers.size(), std::move(cb));
  for (auto* const provider : filters_providers) {
    task_tracker_.PostTask(
        base::SequencedTaskRunner::GetCurrentDefault().get(), FROM_HERE,
        base::BindOnce(&AdBlockFiltersProvider::LoadFilterSet,
                       provider->AsWeakPtr(), std::move(collect_and_merge)));
  }
}

}  // namespace brave_shields
