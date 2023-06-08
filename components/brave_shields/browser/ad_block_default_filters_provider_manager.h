/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_FILTERS_PROVIDER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_FILTERS_PROVIDER_MANAGER_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/singleton.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

// AdBlockDefaultFiltersProviderManager is both an AdBlockFiltersProvider and an
// AdBlockFiltersProvider::Observer. It is used to observe multiple provider
// sources and combine their filter lists into a single compound filter list.
class AdBlockDefaultFiltersProviderManager
    : public AdBlockFiltersProvider,
      public AdBlockFiltersProvider::Observer {
 public:
  AdBlockDefaultFiltersProviderManager(
      const AdBlockDefaultFiltersProviderManager&) = delete;
  AdBlockDefaultFiltersProviderManager& operator=(
      const AdBlockDefaultFiltersProviderManager&) = delete;

  static AdBlockDefaultFiltersProviderManager* GetInstance();

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

  // AdBlockFiltersProvider::Observer
  void OnChanged() override;

  void AddProvider(AdBlockFiltersProvider* provider);
  void RemoveProvider(AdBlockFiltersProvider* provider);

 private:
  friend struct base::DefaultSingletonTraits<
      AdBlockDefaultFiltersProviderManager>;

  void FinishCombinating(
      base::OnceCallback<void(bool, const DATFileDataBuffer&)> cb,
      const std::vector<DATFileDataBuffer>& results);
  base::flat_set<AdBlockFiltersProvider*> filters_providers_;

  base::CancelableTaskTracker task_tracker_;

  base::WeakPtrFactory<AdBlockDefaultFiltersProviderManager> weak_factory_{
      this};

  AdBlockDefaultFiltersProviderManager();
  ~AdBlockDefaultFiltersProviderManager() override;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_DEFAULT_FILTERS_PROVIDER_MANAGER_H_
