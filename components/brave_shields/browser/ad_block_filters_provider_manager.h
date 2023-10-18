/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTERS_PROVIDER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTERS_PROVIDER_MANAGER_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

using brave_component_updater::DATFileDataBuffer;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

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
  AdBlockFiltersProviderManager(const AdBlockFiltersProviderManager&) = delete;
  AdBlockFiltersProviderManager& operator=(
      const AdBlockFiltersProviderManager&) = delete;

  static AdBlockFiltersProviderManager* GetInstance();

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

  void LoadDATBufferForEngine(
      bool is_for_default_engine,
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)> cb);

  // AdBlockFiltersProvider::Observer
  void OnChanged(bool is_default_engine) override;

  void AddProvider(AdBlockFiltersProvider* provider,
                   bool is_for_default_engine);
  void RemoveProvider(AdBlockFiltersProvider* provider,
                      bool is_for_default_engine);

  std::string GetNameForDebugging() override;

 private:
  friend base::NoDestructor<AdBlockFiltersProviderManager>;

  void FinishCombinating(
      base::OnceCallback<void(bool, const DATFileDataBuffer&)> cb,
      const std::vector<DATFileDataBuffer>& results);

  base::flat_set<AdBlockFiltersProvider*> default_engine_filters_providers_;
  base::flat_set<AdBlockFiltersProvider*> additional_engine_filters_providers_;

  base::CancelableTaskTracker task_tracker_;

  base::WeakPtrFactory<AdBlockFiltersProviderManager> weak_factory_{this};

  AdBlockFiltersProviderManager();
  ~AdBlockFiltersProviderManager() override;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTERS_PROVIDER_MANAGER_H_
