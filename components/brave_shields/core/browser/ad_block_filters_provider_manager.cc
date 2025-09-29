// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include <string>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/location.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/services/brave_shields/mojom/filter_set.mojom-forward.h"
#include "brave/components/services/brave_shields/mojom/filter_set.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace {

// Translates the 2-argument `LoadFilters` callback into one that takes a single
// tuple argument. Intended for compatibility with `BarrierCallback`.
void LoadFiltersTuple(base::RepeatingCallback<void(
                          std::tuple<std::vector<unsigned char>, uint8_t>)> cb,
                      std::vector<unsigned char> filter_buffer,
                      uint8_t permission_mask) {
  cb.Run(std::tuple(filter_buffer, permission_mask));
}

}  // namespace

namespace brave_shields {

AdBlockFiltersProviderManager::AdBlockFiltersProviderManager(
    mojo::Remote<filter_set::mojom::UtilParseFilterSet> filter_set_service)
    : filter_set_service_(std::move(filter_set_service)) {}

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
void AdBlockFiltersProviderManager::LoadFilters(
    base::OnceCallback<void(std::vector<unsigned char> filter_buffer,
                            uint8_t permission_mask)>) {
  NOTREACHED();
}

void AdBlockFiltersProviderManager::LoadFiltersForEngine(
    bool is_for_default_engine,
    base::OnceCallback<
        void(const std::vector<unsigned char>& verified_engine_dat)> cb) {
  const uint64_t flow_id = base::RandUint64();
  TRACE_EVENT("brave.adblock",
              "AdBlockFiltersProviderManager::LoadFiltersForEngine",
              perfetto::Flow::ProcessScoped(flow_id), "is_default_engine",
              is_for_default_engine);
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  auto collect_and_merge =
      base::BarrierCallback<std::tuple<std::vector<unsigned char>, uint8_t>>(
          filters_providers.size(),
          base::BindOnce(&AdBlockFiltersProviderManager::FinishCombinating,
                         weak_factory_.GetWeakPtr(), std::move(cb), flow_id));
  for (auto* const provider : filters_providers) {
    task_tracker_.PostTask(
        base::SequencedTaskRunner::GetCurrentDefault().get(), FROM_HERE,
        base::BindOnce(
            &AdBlockFiltersProvider::LoadFilters, provider->AsWeakPtr(),
            base::BindRepeating(LoadFiltersTuple, collect_and_merge)));
  }
}

void AdBlockFiltersProviderManager::FinishCombinating(
    base::OnceCallback<
        void(const std::vector<unsigned char>& verified_engine_dat)> cb,
    uint64_t flow_id,
    std::vector<std::tuple<std::vector<unsigned char>, uint8_t>> results) {
  TRACE_EVENT("brave.adblock",
              "AdBlockFiltersProviderManager::FinishCombinating",
              perfetto::TerminatingFlow::ProcessScoped(flow_id));

  std::vector<filter_set::mojom::FilterListInputPtr> inputs;
  std::transform(results.cbegin(), results.cend(), std::back_inserter(inputs),
                 [](std::tuple<std::vector<unsigned char>, uint8_t> tuple) {
                   auto r = filter_set::mojom::FilterListInput::New();
                   r->filters = std::get<0>(tuple);
                   r->permission_mask = std::get<1>(tuple);
                   return r;
                 });
  filter_set_service_->ParseFilters(std::move(inputs), std::move(cb));
}

}  // namespace brave_shields
