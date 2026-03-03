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
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "brave/components/services/brave_shields/mojom/adblock_filter_list_parser.mojom-forward.h"
#include "brave/components/services/brave_shields/mojom/adblock_filter_list_parser.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace {

// Translates the 3-argument `LoadFilters` callback into one that takes a single
// tuple argument. Intended for compatibility with `BarrierCallback`.
void LoadFiltersTuple(
    base::RepeatingCallback<void(
        std::tuple<mojo_base::BigBuffer,
                   uint8_t,
                   base::OnceCallback<void(adblock::FilterListMetadata)>>)> cb,
    mojo_base::BigBuffer filter_buffer,
    uint8_t permission_mask,
    base::OnceCallback<void(adblock::FilterListMetadata)> on_metadata) {
  cb.Run(std::tuple(std::move(filter_buffer), permission_mask,
                    std::move(on_metadata)));
}

}  // namespace

namespace brave_shields {

AdBlockFiltersProviderManager::AdBlockFiltersProviderManager(
    FilterParsingServiceFactory filter_set_service_factory)
    : list_parser_service_(filter_set_service_factory.Run()) {}

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
    base::OnceCallback<void(
        mojo_base::BigBuffer filter_buffer,
        uint8_t permission_mask,
        base::OnceCallback<void(adblock::FilterListMetadata)> on_metadata)>) {
  NOTREACHED();
}

void AdBlockFiltersProviderManager::LoadFiltersForEngine(
    bool is_for_default_engine,
    base::OnceCallback<void(mojo_base::BigBuffer verified_engine_dat)> cb) {
  const uint64_t flow_id = base::RandUint64();
  TRACE_EVENT("brave.adblock",
              "AdBlockFiltersProviderManager::LoadFiltersForEngine",
              perfetto::Flow::ProcessScoped(flow_id), "is_default_engine",
              is_for_default_engine);
  auto& filters_providers = is_for_default_engine
                                ? default_engine_filters_providers_
                                : additional_engine_filters_providers_;
  auto collect_and_merge = base::BarrierCallback<
      std::tuple<mojo_base::BigBuffer, uint8_t,
                 base::OnceCallback<void(adblock::FilterListMetadata)>>>(
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
    base::OnceCallback<void(mojo_base::BigBuffer verified_engine_dat)> cb,
    uint64_t flow_id,
    std::vector<
        std::tuple<mojo_base::BigBuffer,
                   uint8_t,
                   base::OnceCallback<void(adblock::FilterListMetadata)>>>
        results) {
  TRACE_EVENT("brave.adblock",
              "AdBlockFiltersProviderManager::FinishCombinating",
              perfetto::TerminatingFlow::ProcessScoped(flow_id));

  std::vector<adblock_filter_list_parser::mojom::FilterListInputPtr> inputs;
  inputs.reserve(results.size());
  std::vector<base::OnceCallback<void(adblock::FilterListMetadata)>>
      on_metadata_cbs;
  on_metadata_cbs.reserve(results.size());
  for (auto& [filters, permission_mask, on_metadata_cb] : results) {
    auto r = adblock_filter_list_parser::mojom::FilterListInput::New();
    r->filters = std::move(filters);
    r->permission_mask = permission_mask;
    inputs.push_back(std::move(r));
    on_metadata_cbs.push_back(std::move(on_metadata_cb));
  }
  list_parser_service_->ParseFilters(
      std::move(inputs),
      base::BindOnce(&AdBlockFiltersProviderManager::OnParseFilters,
                     weak_factory_.GetWeakPtr(), std::move(cb),
                     std::move(on_metadata_cbs)));
}

void AdBlockFiltersProviderManager::OnParseFilters(
    base::OnceCallback<void(mojo_base::BigBuffer verified_engine_dat)> cb,
    std::vector<base::OnceCallback<void(adblock::FilterListMetadata)>>
        on_metadata_cbs,
    mojo_base::BigBuffer verified_engine_dat,
    const std::vector<adblock_filter_list_parser::mojom::FilterListMetadataPtr>
        metadata) {
  DCHECK_EQ(on_metadata_cbs.size(), metadata.size());
  for (size_t i = 0; i < metadata.size(); i++) {
    auto adblock_metadata = adblock::FilterListMetadata();
    if (metadata[i]->title) {
      adblock_metadata.title =
          adblock::OptionalString(true, *metadata[i]->title);
    }
    if (metadata[i]->homepage) {
      adblock_metadata.homepage =
          adblock::OptionalString(true, *metadata[i]->homepage);
    }
    if (metadata[i]->expires_hours) {
      adblock_metadata.expires_hours =
          adblock::OptionalU16(true, *metadata[i]->expires_hours);
    }
    std::move(on_metadata_cbs[i]).Run(adblock_metadata);
  }
  std::move(cb).Run(std::move(verified_engine_dat));
}

}  // namespace brave_shields
