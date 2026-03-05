// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_FILTERS_PROVIDER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_FILTERS_PROVIDER_MANAGER_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/services/brave_shields/filter_parsing_service.h"
#include "brave/components/services/brave_shields/mojom/adblock_filter_list_parser.mojom.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/remote.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

// AdBlockFiltersProviderManager is both an AdBlockFiltersProvider and an
// AdBlockFiltersProvider::Observer. It is used to observe multiple provider
// sources and combine their filter lists into a single compound filter list.
// Note that AdBlockFiltersProviderManager should technically not inherit from
// AdBlockFiltersProvider since it manages multiple providers and is not a
// filters provider itself. However, SourceProviderObserver needs it to be so
// for now because AdBlockFiltersProviderManager cannot be used for combining
// DAT files.
class AdBlockFiltersProviderManager : public AdBlockFiltersProvider,
                                      public AdBlockFiltersProvider::Observer {
 public:
  explicit AdBlockFiltersProviderManager(
      FilterParsingServiceFactory filter_set_service_factory);
  ~AdBlockFiltersProviderManager() override;
  AdBlockFiltersProviderManager(const AdBlockFiltersProviderManager&) = delete;
  AdBlockFiltersProviderManager& operator=(
      const AdBlockFiltersProviderManager&) = delete;

  void LoadFilters(
      base::OnceCallback<void(
          mojo_base::BigBuffer filter_buffer,
          uint8_t permission_mask,
          base::OnceCallback<void(adblock::FilterListMetadata)> on_metadata)>)
      override;

  void LoadFiltersForEngine(
      bool is_for_default_engine,
      base::OnceCallback<void(mojo_base::BigBuffer verified_engine_dat)>);

  // AdBlockFiltersProvider::Observer
  void OnChanged(bool is_default_engine) override;

  void AddProvider(AdBlockFiltersProvider* provider,
                   bool is_for_default_engine);
  void RemoveProvider(AdBlockFiltersProvider* provider,
                      bool is_for_default_engine);

  std::string GetNameForDebugging() override;

 private:
  void OnParseFilters(
      base::OnceCallback<void(mojo_base::BigBuffer verified_engine_dat)> cb,
      std::vector<base::OnceCallback<void(adblock::FilterListMetadata)>>
          on_metadata_cbs,
      mojo_base::BigBuffer verified_engine_dat,
      const std::vector<adblock::mojom::FilterListMetadataPtr> metadata);
  void FinishCombinating(
      base::OnceCallback<void(mojo_base::BigBuffer verified_engine_dat)> cb,
      uint64_t flow_id,
      std::vector<
          std::tuple<mojo_base::BigBuffer,
                     uint8_t,
                     base::OnceCallback<void(adblock::FilterListMetadata)>>>
          results);

  base::flat_set<AdBlockFiltersProvider*> default_engine_filters_providers_;
  base::flat_set<AdBlockFiltersProvider*> additional_engine_filters_providers_;

  base::CancelableTaskTracker task_tracker_;

  mojo::Remote<adblock::mojom::AdblockFilterListParser> list_parser_service_;

  base::WeakPtrFactory<AdBlockFiltersProviderManager> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_FILTERS_PROVIDER_MANAGER_H_
